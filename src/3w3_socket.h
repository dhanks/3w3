#define SOCKET_SHUTDOWN		1<<0

struct s_packets
{
	unsigned long bytes, errors, dropped, packets;
};

typedef struct _socket
{
        unsigned int flags;
        unsigned int telnet;
        int port;
        int fd;
	socklen_t addrlen;
	size_t len;
        char ip[sizeof("xxx.xxx.xxx.xxx") + 1];
        char buf[BUFSIZ];
        char name[BUFSIZ];
        char host[BUFSIZ];
        char serv[BUFSIZ];
        struct sockaddr_un udp;
	struct sockaddr_in in;
	struct addrinfo hints;
	struct addrinfo *res, *ressave;
        struct s_packets pin, pout;
	fd_set fds;
	fd_set client_fds;
	struct timeval tv;
	list_t *list;
	list_t *plist;
	node_t *node;
	int (*f_loop) (void *);
	int (*f_disconnect) (void *);
	int (*f_read) (void *, const char *, size_t);
	void * (*f_connect)(void *);
	void * (*f_accept)(void *);
	pthread_t thread;
	pthread_mutex_t lock;
} socket_t;

typedef struct _dns
{
	char ip[sizeof("xxx.xxx.xxx.xxx") + 1];
	char dns[BUFSIZ];
	time_t t;
} dns_t;

extern const char *socket_ip2host(const char *ip);
extern size_t socket_write(socket_t *s, const char *buf);
extern size_t socket_prompt(socket_t *s, const char *buf);
extern size_t socket_writef(socket_t *s, const char *format, ...);
extern size_t socket_read(int fd, void *buf, size_t count);
extern int socket_read_print(void *arg);
extern void *socket_pthread_per_accept(void *arg);
extern void *socket_select(void *arg);
extern socket_t *socket_new(void);
