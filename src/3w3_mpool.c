#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stddef.h>

#include "3w3_list.h"
#include "3w3_mpool.h"
#include "3w3_alloc.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

static long pagesize;

#define align(s,a) (((s)+(a)-1) & ~((a)-1))

static void mp_lock(mempool_t *mp)
{
	if(mp->locking)
		pthread_mutex_lock(&mp->lock);
}

static void mp_unlock(mempool_t *mp)
{
	if(mp->locking)
		pthread_mutex_unlock(&mp->lock);
}

static void mp_free(void *p)
{
	mempool_t *mp = p;

	pthread_mutex_destroy(&mp->lock);
}

extern mempool_t *mempool_new(size_t size, int lock)
{
	mempool_t *mp;

	if(!pagesize)
		pagesize = sysconf(_SC_PAGESIZE);

	size = align(size, sizeof(void *));

	if(size > pagesize - offsetof(mempool_page_t, data))
		return NULL;

	mp = allocdz(sizeof(*mp), NULL, mp_free);
	mp->size = size;
	mp->cpp = (pagesize - offsetof(mempool_page_t, data)) / size;
	mp->locking = lock;
	pthread_mutex_init(&mp->lock, NULL);

	return mp;
}

extern void *mempool_get(mempool_t *mp)
{
	mempool_page_t *mpp;
	void *chunk;

	mp_lock(mp);

	mpp = mp->pages;

	if(!mpp)
	{
		mpp = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		mpp->pool = mp;
		mp->pages = mpp;
	}

	if(mpp->free)
	{
		chunk = mpp->free;
		mpp->free = *(void **) chunk;
	}
	else
	{
		chunk = (char *) mpp->data + mpp->used * mp->size;
		mpp->used++;
	}

	if(++mpp->inuse == mp->cpp)
	{
		mp->pages = mpp->next;
		mpp->next = NULL;
		mpp->prev = NULL;
	}

	mp_unlock(mp);

	return chunk;
}

extern void mempool_free(void *p)
{
	mempool_page_t *mpp = (void *) ((ptrdiff_t) p & ~(pagesize - 1));
	mempool_t *mp = mpp->pool;

	mp_lock(mp);

	if(!--mpp->inuse)
	{
		if(mp->pages == mpp)
			mp->pages = mpp->next;
		if(mpp->next)
			mpp->next->prev = mpp->prev;
		if(mpp->prev)
			mpp->prev->next = mpp->next;

		munmap(mpp, pagesize);
	}
	else
	{
		*(void **) p = mpp->free;

		mpp->free = p;

		if(!mpp->next && !mpp->prev && mp->pages != mpp)
		{
			if(mp->pages)
				mp->pages->prev = mpp;

			mpp->next = mp->pages;
			mp->pages = mpp;
		}
	}

	mp_unlock(mp);
}
