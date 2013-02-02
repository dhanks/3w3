typedef void (*ref_fn)(void *);
typedef void *(*attr_ref_t)(void *);

typedef struct _attri {
    char *name;
    void *value;
    attr_ref_t ref;
    free_fn free;
    struct _attri *next;
} attri_t;

typedef struct _alloc {
    long rc;
    ref_fn ref;
    free_fn free;
    attri_t *attr;
    union {
        long l;
        double d;
        void *p;
    } data[1];
} alloc_t;


typedef struct attr {
    char *name;
    void *value;
} attr_t;


extern void *allocd(size_t, ref_fn, free_fn);
extern void *allocdz(size_t, ref_fn, free_fn);
extern void *ref(void *);
extern void FREE(void *);
extern int attr_set(void *p, char *name, void *val, attr_ref_t r, free_fn f);
extern void *attr_get(void *p, char *name);
extern int attr_getall(void *ptr, int n, attr_t *attr);
extern int attr_del(void *p, char *name);
