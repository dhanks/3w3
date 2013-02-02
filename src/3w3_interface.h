void *interface_init(void *arg);
int interface_tcp_create(void *(*f_accept)(void *), void *(*f_connect)(void *), const char *host, const char *serv);
