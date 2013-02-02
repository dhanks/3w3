/* ======================================================================= */
/* list structure */
/* ======================================================================= */
typedef void (*free_fn) (void *);

typedef struct node
{
	void *data;
	struct node *next;
	struct node *prev;
	free_fn free;
	int rc;
	int ic;
	int deleted;
} node_t;

typedef struct list
{
	struct node *start;
	struct node *end;
	unsigned long items;
	unsigned long deleted;
	int locking;
	pthread_mutex_t lock;
} list_t;

/* ======================================================================= */
/* public functions declarations */
/* ======================================================================= */
extern list_t *list_new(void);
extern int list_free(list_t *list);
extern int list_unlock_deref(list_t *list, node_t *node);
extern int list_delete_data(list_t *list, void *p);
extern void list_remove(list_t *list, node_t *node, free_fn fr);
extern int list_destroy(list_t *list, free_fn lfree);
extern int list_push(list_t *list, void *p);
extern int list_unshift(list_t *list, void *p);
extern unsigned long list_items(list_t *lst);
extern void *list_shift(list_t *list);
extern void *list_next(list_t *list, node_t **node);
extern void *list_prev(list_t *list, node_t **node);
extern void *list_pop(list_t *list);
