typedef enum _dfile_type
{
	DFILE_ISSUE = 0,
	DFILE_MOTD,
	DFILE_END
} dfile_type_t;

typedef struct _dfile
{
	char path[BUFSIZ];
	char *data;
	unsigned long size;
	unsigned long flags;
	unsigned int accessed;
	unsigned int reloaded;
	time_t loaded;
	time_t last_accessed;
	time_t last_reloaded;
	pthread_mutex_t lock;
} dfile_t;

extern int dfile_new(dfile_type_t type, const char *path);
extern char *dfile_get(dfile_type_t type);
