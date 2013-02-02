#define NEWBIE		(unsigned long)(1<<0)
#define RESIDENT	(unsigned long)(1<<1)
#define MAIL		(unsigned long)(1<<2)
#define NEWS		(unsigned long)(1<<3)
#define GAMES		(unsigned long)(1<<4)
#define PSU		(unsigned long)(1<<5)
#define SU		(unsigned long)(1<<6)
#define ASU		(unsigned long)(1<<7)
#define LOWER_ADMIN	(unsigned long)(1<<8)
#define ADMIN		(unsigned long)(1<<9)
#define CODER		(unsigned long)(1<<10)
#define SYSADMIN	(unsigned long)(1<<11)

#define LOGGED_IN	(unsigned long)(1<<1)
#define LOGOUT		(unsigned long)(1<<2)
#define AWAY		(unsigned long)(1<<3)
#define QUIET		(unsigned long)(1<<4)
#define P_LAST_R	(unsigned long)(1<<5)
#define P_LAST_N	(unsigned long)(1<<6)
#define P_READY		(unsigned long)(1<<7)

typedef struct _screen
{
	size_t width, height, linewrap, wordwrap;
	char type[255];
} screen_t;

typedef struct _player
{
        unsigned long flags;
        unsigned long oflags;
        unsigned long privs;
	char **env;
	char passwd[BUFSIZ];
	char name[BUFSIZ];
	char prompt[BUFSIZ];
	char ibuffer[BUFSIZ];
	int ibuffer_ptr;
	int (*f_mode) (void *, const char *, size_t);
	screen_t term;
	socket_t socket;
	list_t *plist;
	node_t *node;
	pthread_t thread;
	pthread_mutex_t lock;
} player_t;

extern void player_close(player_t *p);
extern player_t *player_new(void);
