#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <assert.h>

#include "3w3_list.h"

#define fail(a) fprintf(stderr, "[%.63s::%.63s, line %i] %.63s: %.127s (%i)\n", __FILE__, __func__, __LINE__, a, strerror(errno), errno)

/* ANSI C doesn't support "static inline" so we'll use the preprocessor */
#define list_ref(node) node->rc++

/* ======================================================================= */
/* private functions declarations */
/* ======================================================================= */
static int list_unlink(list_t *list, node_t *node);
static node_t *node_new(void *p);
static void list_deref(list_t *list, node_t *node);

/* ======================================================================= */
/* public functions */
/* ======================================================================= */

extern int list_delete_data(list_t *list, void *p)
{
	int n = 0;
	node_t *node = NULL;

	for(list_next(list, &node); node != NULL; list_next(list, &node))
	{
		if(p == node->data)
		{
			list_remove(list, node, NULL);
			n++;
		}
	}

	return(n);
}


extern void *list_prev(list_t *list, node_t **node)
{
	void *r = NULL;

	assert(list);

	pthread_mutex_lock(&list->lock);

	do
	{
		if(*node == NULL)
		{
			if(list->end != NULL)
				list_ref(list->end);
			*node = list->end;
		}
		else
		{
			node_t *ln;

			if((*node)->prev != NULL)
			{
				list_ref((*node)->prev);
			}

			ln = (*node)->prev;
			(*node)->ic--;
			list_deref(list, *node);
			*node = ln;
		}

		if(*node != NULL)
		{
			r = (*node)->data;
			(*node)->ic++;
		}

	} while(*node && (*node)->deleted);

	pthread_mutex_unlock(&list->lock);
	return *node? r: NULL;
}

extern void *list_pop(list_t *list)
{
	node_t *node = NULL;
	void *data = NULL;

	assert(list);
	list_prev(list, &node);

	if(node)
	{
		data = node->data;
		list_remove(list, node, NULL);
		list_unlock_deref(list, node);
	}

	return(data);
}

extern void *list_shift(list_t *list)
{
	node_t *node = NULL;
	void *data = NULL;

	assert(list);
	list_next(list, &node);

	if(node)
	{
		data = node->data;
		list_remove(list, node, NULL);
		list_unlock_deref(list, node);
	}

	return(data);
}


extern void *list_next(list_t *list, node_t **node)
{
	void *r = NULL;

	assert(list);
	pthread_mutex_lock(&list->lock);

	do
	{
		if(*node == NULL)
		{
			if(list->start != NULL)
				list_ref(list->start);
			*node = list->start;
		}
		else
		{
			node_t *ln;

			if((*node)->next != NULL)
			{
				list_ref((*node)->next);
			}

			ln = (*node)->next;
			(*node)->ic--;
			list_deref(list, *node);
			*node = ln;
		}

		if(*node != NULL)
		{
			r = (*node)->data;
			(*node)->ic++;
		}
	} while(*node && (*node)->deleted);

	pthread_mutex_unlock(&list->lock);
	return *node? r: NULL;
}

extern int list_unshift(list_t *list, void *p)
{
	node_t *node = node_new(p);

	assert(list);
	assert(node);
	assert(p);

	pthread_mutex_lock(&list->lock);

	if(list->start == NULL)
	{
		list->start = node;
		list->start->next = NULL;
		list->start->prev = NULL;
		list->end = list->start;
	}
	else
	{
		list->start->prev = node;
		node->next = list->start;
		node->prev = NULL;
		list->start = node;
	}

	list->items++;
	pthread_mutex_unlock(&list->lock);
	return(0);
}

extern int list_push(list_t *list, void *p)
{
	node_t *node = node_new(p);

	assert(list);
	assert(node);
	assert(p);

	pthread_mutex_lock(&list->lock);

	if(list->start == NULL)
	{
		list->start = node;
		list->start->next = NULL;
		list->start->prev = NULL;
		list->end = list->start;
	}
	else
	{
		list->end->next = node;
		node->prev = list->end;
		node->next = NULL;
		list->end = node;
	}
	list->items++;
	pthread_mutex_unlock(&list->lock);
	return(0);
}

extern list_t *list_new(void)
{
	list_t *l;

 	l = calloc(1, sizeof(list_t));
	assert(l);

	pthread_mutex_init(&l->lock, NULL);
	return(l);
}

extern int list_free(list_t *list)
{
	assert(list);

	if(list->start != NULL)
		return -1;

	pthread_mutex_destroy(&list->lock);
	free(list);
	return 0;
}

extern void list_remove(list_t *list, node_t *node, free_fn fr)
{
	assert(list);
	assert(node);

	pthread_mutex_lock(&list->lock);
	node->deleted = 1;
	node->free = fr;
	list->deleted++;
	list_deref(list, node);
	pthread_mutex_unlock(&list->lock);
}

extern int list_destroy(list_t *list, free_fn lfree)
{
	assert(list);
	pthread_mutex_lock(&list->lock);

	while(list->start)
	{
		list->start->free = lfree;
		list_unlink(list, list->start);
	}

	pthread_mutex_unlock(&list->lock);
	list_free(list);
	return(0);
}

extern int list_unlock_deref(list_t *list, node_t *node)
{
	assert(list);
	assert(node);

	pthread_mutex_lock(&list->lock);
	node->ic--;
	list_deref(list, node);
	pthread_mutex_unlock(&list->lock);
	return(0);
}

/* ======================================================================= */
/* private functions */
/* ======================================================================= */

static void list_deref(list_t *list, node_t *node)
{
	assert(list);
	assert(node);
	if(--node->rc <= 0 && node->ic <= 0)
	list_unlink(list, node);
}

static node_t *node_new(void *p)
{
	node_t *node = calloc(1, sizeof(node_t));

	assert(p);
	assert(node);

	node->data = p;
	node->rc = 1;
	node->ic = 0;
	node->free = NULL;
	node->deleted = 0;
	return(node);
}

static int list_unlink(list_t *list, node_t *node)
{
	assert(list);
	assert(node);

	if(node->prev)
		node->prev->next = node->next;

	if(node->next)
		node->next->prev = node->prev;

	if(node == list->start)
		list->start = node->next;

	if(node == list->end)
		list->end = node->prev;

	if(node->free)
		node->free(node->data);

	if(node->deleted)
		list->deleted--;

	list->items--;
	free(node);
	return(0);
}

extern unsigned long list_items(list_t *lst)
{
    return lst->items - lst->deleted;
}
