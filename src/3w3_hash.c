#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "3w3_list.h"
#include "3w3_mpool.h"
#include "3w3_hash.h"

static void lock_hash(hash_table_t *ht)
{
	if(ht->locking)
		pthread_mutex_lock(&ht->lock);
}

static void unlock_hash(hash_table_t *ht)
{
	if(ht->locking)
		pthread_mutex_unlock(&ht->lock);
}

static int hash_cmp(void *key, size_t ks, hash_entry *he)
{
	if(ks != he->key_size)
		return 1;

	if(key == he->key)
		return 0;

	return memcmp(key, he->key, ks);
}

static void * hash_kdup(void *key, size_t ks)
{
	void *nk;

	if((nk = malloc(ks)) == NULL)
		return(NULL);

	memcpy(nk, key, ks);
	return nk;
}

static hash_entry * hash_addentry(hash_table_t *ht, unsigned int hv, void *key, size_t ks, void *data)
{
	hash_entry *he = mempool_get(ht->mp);

	if(ht->flags & HASH_NOCOPY)
		he->key = key;
	else
		he->key = hash_kdup(key, ks);

	he->key_size = ks;
	he->data = data;
	he->next = ht->buckets[hv];
	ht->buckets[hv] = he;
	ht->entries++;

	return he;
}

static unsigned int hash_size(unsigned int s) 
{ 
	unsigned int i = 1;

	while(i < s)
		i <<= 1;
	return(i);
} 

static unsigned int hash_func(void *key, size_t length)
{
	register unsigned int a,b,c,len;
	char *k = key;

	len = length;
	a = b = 0x9e3779b9;
	c = 0;

	while(len >= 12)
	{
		a += (k[0] +((unsigned int)k[1]<<8) +((unsigned int)k[2]<<16) +((unsigned int)k[3]<<24));
		b += (k[4] +((unsigned int)k[5]<<8) +((unsigned int)k[6]<<16) +((unsigned int)k[7]<<24));
		c += (k[8] +((unsigned int)k[9]<<8) +((unsigned int)k[10]<<16)+((unsigned int)k[11]<<24));
		mix(a,b,c);
		k += 12; len -= 12;
	}

	c += length;

	switch(len)
	{
		case 11:
			c+=((unsigned int)k[10]<<24);
		case 10:
			c+=((unsigned int)k[9]<<16);
		case 9:
			c+=((unsigned int)k[8]<<8);
		case 8:
			b+=((unsigned int)k[7]<<24);
		case 7:
			b+=((unsigned int)k[6]<<16);
		case 6:
			b+=((unsigned int)k[5]<<8);
		case 5:
			b+=k[4];
		case 4:
			a+=((unsigned int)k[3]<<24);
		case 3:
			a+=((unsigned int)k[2]<<16);
		case 2:
			a+=((unsigned int)k[1]<<8);
		case 1:
			a+=k[0];
	}

	mix(a,b,c);
	return c;
}

static unsigned int hash_value(hash_table_t *ht, void *key, size_t ks, size_t size)
{
	unsigned int hv = ht->hash_func(key, ks);
	return hv & (size - 1);
}

extern hash_table_t *hash_new(int size, int lock, unsigned int flags)
{
	hash_table_t *ht;

	size = hash_size(size);

	if((ht = malloc(sizeof(*ht))) == NULL)
		return(NULL);

	ht->size = size;
	ht->entries = 0;
	ht->flags = flags;
	ht->buckets = calloc(size, sizeof(*ht->buckets));
	ht->locking = lock;
	pthread_mutex_init(&ht->lock, NULL);
	ht->high_mark = 0.7;
	ht->low_mark = 0.3;
	ht->hash_func = hash_func;
	ht->mp = mempool_new(sizeof(hash_entry), 0);

	return ht;
}

extern int hash_sethashfunction(hash_table_t *ht, hash_function_t hf)
{
	if(ht->entries)
		return -1;

	ht->hash_func = hf? hf: hash_func;
	return 0;
}

extern int hash_search(hash_table_t *ht, void *key, size_t ks, void *data, void *r)
{
	unsigned int hv;
	hash_entry *hr;
	void **ret = r;
	int hf = 0;

	if(ks == (size_t) -1)
		ks = strlen(key) + 1;

	lock_hash(ht);

	hv = hash_value(ht, key, ks, ht->size);

	for(hr = ht->buckets[hv]; hr; hr = hr->next)
		if(!hash_cmp(key, ks, hr))
			break;

	if(!hr)
	{
		hr = hash_addentry(ht, hv, key, ks, data);
		hf = 1;
	}

	if(ret != NULL)
		*ret = hr->data;

	unlock_hash(ht);

	if(hf && !(ht->flags & HASH_FROZEN))
	{
		if((float) ht->entries / ht->size > ht->high_mark)
			hash_rehash(ht);
	}

	return hf;
}

extern int hash_find(hash_table_t *ht, void *key, size_t ks, void *r)
{
	unsigned int hv;
	hash_entry *hr = NULL;
	void **ret = r;

	if(ks == (size_t) -1)
		ks = strlen(key) + 1;

	lock_hash(ht);
	hv = hash_value(ht, key, ks, ht->size);

	for(hr = ht->buckets[hv]; hr; hr = hr->next)
		if(!hash_cmp(key, ks, hr))
			break;

	if(hr && ret)
		*ret = hr->data;

	unlock_hash(ht);
	return !hr;
}


extern int hash_replace(hash_table_t *ht, void *key, size_t ks, void *data, void *r)
{
	unsigned int hv;
	hash_entry *hr;
	int ret = 0;
	void **rt = r;

	if(ks == (size_t) -1)
		ks = strlen(key) + 1;

	lock_hash(ht);
	hv = hash_value(ht, key, ks, ht->size);

	for(hr = ht->buckets[hv]; hr; hr = hr->next)
		if(!hash_cmp(key, ks, hr))
			break;

	if(hr)
	{
		if(rt)
			*rt = hr->data;

		hr->data = data;
	}
	else
	{
		hash_addentry(ht, hv, key, ks, data);
		ret = 1;
	}

	unlock_hash(ht);

	if(ret && !(ht->flags & HASH_FROZEN))
	{
		if((float) ht->entries / ht->size > ht->high_mark)
			hash_rehash(ht);
	}

	return ret;
}


extern int hash_delete(hash_table_t *ht, void *key, size_t ks, void *r)
{
	int hv;
	hash_entry *hr = NULL, *hp = NULL;
	void **ret = r;

	if(ks == (size_t) -1)
		ks = strlen(key) + 1;

	lock_hash(ht);
	hv = hash_value(ht, key, ks, ht->size);

	for(hr = ht->buckets[hv]; hr; hr = hr->next)
	{
		if(!hash_cmp(key, ks, hr))
			break;

		hp = hr;
	}

	if(hr)
	{
		if(ret)
			*ret = hr->data;
		if(hp)
			hp->next = hr->next;
		else
			ht->buckets[hv] = hr->next;

		ht->entries--;

		if(!(ht->flags & HASH_NOCOPY))
			free(hr->key);

		mempool_free(hr);
	}

	unlock_hash(ht);

	if(!hr && !(ht->flags & HASH_FROZEN))
	{
		if((float) ht->entries / ht->size < ht->low_mark)
			hash_rehash(ht);
	}

	return !hr;
}

extern int hash_destroy(hash_table_t *ht, free_fn hf)
{
	size_t i;

	for(i = 0; i < ht->size; i++)
	{
		if(ht->buckets[i] != NULL)
		{
			hash_entry *he = ht->buckets[i];

			while(he)
			{
				hash_entry *hn = he->next;

				if(hf)
					hf(he->data);
				if(!(ht->flags & HASH_NOCOPY))
					free(he->key);

				mempool_free(he);
				he = hn;
			}
		}
	}

	free(ht->buckets);
	pthread_mutex_destroy(&ht->lock);
	free(ht->mp);
	free(ht);

	return 0;
}

extern int hash_rehash(hash_table_t *ht)
{
	hash_entry **nb = NULL;
	size_t ns, i;

	lock_hash(ht);

	ns = hash_size(ht->entries * 2 / (ht->high_mark + ht->low_mark));

	if(ns == ht->size)
		goto hash_rehash_end;

	nb = calloc(ns, sizeof(*nb));

	for(i = 0; i < ht->size; i++)
	{
		hash_entry *he = ht->buckets[i];

		while(he)
		{
			hash_entry *hn = he->next;
			int hv = hash_value(ht, he->key, he->key_size, ns);

			he->next = nb[hv];
			nb[hv] = he;
			he = hn;
		}
	}

	ht->size = ns;
	free(ht->buckets);
	ht->buckets = nb;

	hash_rehash_end:
	unlock_hash(ht);
	return 0;
}

extern void **hash_keys(hash_table_t *ht, int *n, int fast)
{
	void **keys;
	size_t i, j;

	if(ht->entries == 0)
	{
		*n = 0;
		return NULL;
	}

	lock_hash(ht);

	if((keys = malloc(ht->entries * sizeof(*keys))) == NULL)
		return(NULL);

	for(i = 0, j = 0; i < ht->size; i++)
	{
		hash_entry *he;

		for(he = ht->buckets[i]; he; he = he->next)
			keys[j++] = fast? he->key: hash_kdup(he->key, he->key_size);
	}

	*n = ht->entries;
	unlock_hash(ht);

	return keys;
}

extern int hash_setthresholds(hash_table_t *ht, float low, float high)
{
	float h = high < 0? ht->high_mark: high;
	float l = low < 0? ht->low_mark: low;

	if(h < l)
		return -1;

	ht->high_mark = h;
	ht->low_mark = l;

	return 0;
}

extern int hash_getflags(hash_table_t *ht)
{
	return ht->flags;
}

extern int hash_setflags(hash_table_t *ht, int flags)
{
	lock_hash(ht);
	ht->flags = flags;
	unlock_hash(ht);
	return ht->flags;
}

extern int hash_setflag(hash_table_t *ht, int flag)
{
	lock_hash(ht);
	ht->flags |= flag;
	unlock_hash(ht);
	return ht->flags;
}

extern int hash_clearflag(hash_table_t *ht, int flag)
{
	lock_hash(ht);
	ht->flags &= ~flag;
	unlock_hash(ht);
	return ht->flags;
}
