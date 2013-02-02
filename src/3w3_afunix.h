#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

extern int afunix_udp_init(socket_t *udp, const char *name);
