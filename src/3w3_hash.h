#define HASH_FROZEN 0x01  /* Automatic resizing not allowed */
#define HASH_NOCOPY 0x02  /* Don't copy keys */

typedef unsigned int (*hash_function_t)(void *key, size_t size);

typedef struct _hash_entry
{
        void *key;
        size_t key_size;
        void *data;
        struct _hash_entry *next;
} hash_entry;

typedef struct _hash_table
{
        hash_function_t hash_func;
        size_t size;           /* Number of buckets. */
        size_t entries;        /* Number of entries in table. */
        hash_entry **buckets;
        unsigned int flags;
        int locking;
        pthread_mutex_t lock;
        float high_mark, low_mark;
        mempool_t *mp;
} hash_table_t;

#define mix(a,b,c)                              \
{                                               \
  a -= b; a -= c; a ^= (c>>13);                 \
  b -= c; b -= a; b ^= (a<<8);                  \
  c -= a; c -= b; c ^= (b>>13);                 \
  a -= b; a -= c; a ^= (c>>12);                 \
  b -= c; b -= a; b ^= (a<<16);                 \
  c -= a; c -= b; c ^= (b>>5);                  \
  a -= b; a -= c; a ^= (c>>3);                  \
  b -= c; b -= a; b ^= (a<<10);                 \
  c -= a; c -= b; c ^= (b>>15);                 \
}

extern hash_table_t *hash_new(int size, int lock, unsigned int flags);
extern int hash_sethashfunction(hash_table_t *ht, hash_function_t hf);
extern int hash_search(hash_table_t *ht, void *key, size_t ks, void *data, void *r);
extern int hash_find(hash_table_t *ht, void *key, size_t ks, void *r);
extern int hash_replace(hash_table_t *ht, void *key, size_t ks, void *data, void *r);
extern int hash_delete(hash_table_t *ht, void *key, size_t ks, void *r);
extern int hash_destroy(hash_table_t *ht, free_fn hf);
extern int hash_rehash(hash_table_t *ht);
extern void ** hash_keys(hash_table_t *ht, int *n, int fast);
extern int hash_setthresholds(hash_table_t *ht, float low, float high);
extern int hash_getflags(hash_table_t *ht);
extern int hash_setflags(hash_table_t *ht, int flags);
extern int hash_setflag(hash_table_t *ht, int flag);
extern int hash_clearflag(hash_table_t *ht, int flag);
