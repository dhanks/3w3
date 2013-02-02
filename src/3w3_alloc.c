#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "3w3_list.h"
#include "3w3_alloc.h"

static void attr_free(attri_t *a);

extern void *allocd(size_t size, ref_fn r, free_fn f)
{
	alloc_t *tca;

	if((tca = malloc(size + sizeof(alloc_t) - sizeof(tca->data))) == NULL)
		return(NULL);

	tca->rc = 1;
	tca->ref = r;
	tca->free = f;
	tca->attr = NULL;
	return tca->data;
}

extern void *allocdz(size_t size, ref_fn r, free_fn f)
{
	void *p = allocd(size, r, f);

	if(p)
		memset(p, 0, size);

	return p;
}

#define ALLOC(size) allocd(size, NULL, NULL)
#define allocz(size) allocdz(size, NULL, NULL)

extern void *ref(void *ptr)
{
	alloc_t *tca = (alloc_t *)((char *) ptr - offsetof(alloc_t, data));

	tca->rc++;
	if(tca->ref)
		tca->ref(tca->data);
	return ptr;
}

extern void FREE(void *ptr)
{
	alloc_t *tca;

	if(!ptr)
		return;

	tca = (alloc_t *)((char *) ptr - offsetof(alloc_t, data));
	tca->rc--;
	if(!tca->rc)
	{
		attri_t *a;

		if(tca->free)
			tca->free(tca->data);

		for(a = tca->attr; a;)
		{
			attri_t *n = a->next;

			attr_free(a);
			a = n;
		}

		free(tca);
	}
}

static void attr_free(attri_t *a)
{
	free(a->name);
	if(a->free)
		a->free(a->value);
	free(a);
}

extern int attr_set(void *ptr, char *name, void *val, attr_ref_t r, free_fn f)
{
	alloc_t *tca = (alloc_t *)((char *) ptr - offsetof(alloc_t, data));
	attri_t *a, *n, *p = NULL;

	for(a = tca->attr; a && a->next && strcmp(name, a->name); a = a->next)
		p = a;

	n = calloc(1, sizeof(*n));
	n->name = strdup(name);
	n->value = val;
	n->ref = r;
	n->free = f;

	if(!a)
	{
		tca->attr = n;
	}
	else if(strcmp(name, a->name))
	{
		a->next = n;
	}
	else
	{
		n->next = a->next;

		if(p)
			p->next = n;
		else
			tca->attr = n;

		attr_free(a);
	}

	return 0;
}

extern void *attr_get(void *p, char *name)
{
	alloc_t *tca = (alloc_t *)((char *) p - offsetof(alloc_t, data));
	attri_t *a;
	void *v = NULL;

	for(a = tca->attr; a && strcmp(name, a->name); a = a->next);

	if(a)
		v = a->ref? a->ref(a->value): a->value;

	return v;
}

extern int attr_getall(void *p, int n, attr_t *attr)
{
	alloc_t *tca = (alloc_t *)((char *) p - offsetof(alloc_t, data));
	attri_t *a;
	int i;

	for(i = 0, a = tca->attr; a && i < n; a = a->next, i++)
	{
		attr[i].name = a->name;
		attr[i].value = a->ref? a->ref(a->value): a->value;
	}

	return i;
}

extern int attr_del(void *ptr, char *name)
{
	alloc_t *tca = (alloc_t *)((char *) ptr - offsetof(alloc_t, data));
	attri_t *a, *p = NULL;

	for(a = tca->attr; a && strcmp(name, a->name); a = a->next)
		p = a;

	if(a)
	{
		if(p)
			p->next = a->next;
		else
			tca->attr = a->next;

		attr_free(a);
	}

	return 0;
}
