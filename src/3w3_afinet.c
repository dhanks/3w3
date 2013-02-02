#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <pthread.h>

#include "config.h"
#include "../sqlite/sqlite3.h"
#include "3w3_list.h"
#include "3w3_socket.h"
#include "3w3_afunix.h"

/* public functions */
extern int afinet_tcp_listen(socket_t *tcp, const char *host, const char *serv)
{
	const int on = 1;
	int n;

	assert(tcp);
	assert(serv);

	memset(&tcp->hints, '\0', sizeof(struct addrinfo));

	tcp->hints.ai_flags = AI_PASSIVE;
	tcp->hints.ai_family = AF_UNSPEC;
	tcp->hints.ai_socktype = SOCK_STREAM;

	if((n = getaddrinfo(host, serv, &tcp->hints, &tcp->res)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %.100s (%i)\n", gai_strerror(n), n);
		return(-1);
	}

	tcp->ressave = tcp->res;

	do
	{
		if((tcp->fd = socket(tcp->res->ai_family, tcp->res->ai_socktype, tcp->res->ai_protocol)) == -1)
		{
			fprintf(stderr, "listen: %.100s (%i)\n", strerror(errno), errno);
			return(-1);
		}

		if(setsockopt(tcp->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
		{
			fprintf(stderr, "setsockopt: %.100s (%i)\n", strerror(errno), errno);
			return(-1);
		}

		if(bind(tcp->fd, tcp->res->ai_addr, tcp->res->ai_addrlen) == -1)
		{
			fprintf(stderr, "bind: %.100s (%i)\n", strerror(errno), errno);
			return(-1);
		}
	} while((tcp->res = tcp->res->ai_next) != NULL);

	if(listen(tcp->fd, 1024) == -1)
	{
		fprintf(stderr, "bind: %.100s (%i)\n", strerror(errno), errno);
		return(-1);
	}

	tcp->len = tcp->ressave->ai_addrlen;
	freeaddrinfo(tcp->ressave);

	return(tcp->fd);
}
