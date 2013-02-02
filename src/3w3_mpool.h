#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef struct _mempool_page
{
	struct _mempool *pool;
	struct _mempool_page *next, *prev;
	size_t used, inuse;
	void *free;
	union
	{
		long l;
		double d;
		void *p;
	} data[1];
} mempool_page_t;

typedef struct _mempool
{
	size_t size;
	size_t cpp;
	int locking;
	mempool_page_t *pages;
	pthread_mutex_t lock;
} mempool_t;

extern mempool_t *mempool_new(size_t size, int lock);
extern void *mempool_get(mempool_t *mp);
extern void mempool_free(void *p);
