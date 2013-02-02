typedef struct _command
{
	const char *key;
	int (*command) (void *, const char *, size_t);
	unsigned long level;
	unsigned long flags;
} command_t;

extern int command_who(void *arg, const char *input, size_t length);
extern int command_twho(void *arg, const char *input, size_t length);
extern int command_quit(void *arg, const char *input, size_t length);
extern int command_init(void);
extern int command_push(const char *key, int (*command) (void *, const char *, size_t), unsigned long level, unsigned long flags);
extern int command_init(void);
