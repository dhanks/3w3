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
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>

#include "config.h"
#include "../sqlite/sqlite3.h"
#include "3w3_list.h"
#include "3w3_mpool.h"
#include "3w3_hash.h"
#include "3w3_socket.h"
#include "3w3_player.h"
#include "3w3_afunix.h"
#include "3w3_process_input.h"
#include "3w3_login.h"
#include "3w3_dfile.h"
#include "3w3_globals.h"

/* private functions */
static int socket_close(socket_t *s)
{
	pthread_mutex_lock(&s->lock);

	assert(s);

	if(shutdown(s->fd, SHUT_RDWR) == -1)
		fprintf(stderr, "shutdown(%i): %.100s (%i)\n", s->fd, strerror(errno), errno);
	if(close(s->fd) == -1)
		fprintf(stderr, "close(%i): %.100s (%i)\n", s->fd, strerror(errno), errno);

	list_delete_data(socket_list, s);
	list_delete_data(interface_list, s);
	pthread_mutex_unlock(&s->lock);

	free(s);
	return(0);
}

static size_t _socket_write(socket_t *s, const void *buf, size_t count)
{
	size_t n;

	pthread_mutex_lock(&s->lock);

	assert(s);
	assert(buf);
_socket_write_again:
	if((n = write(s->fd, buf, count)) == -1)
		if(errno == EAGAIN || errno == EINTR || errno == EIO)
			goto _socket_write_again;

	pthread_mutex_unlock(&s->lock);
	return(n);
}

static void *_socket_ip2host(void *arg)
{
	dns_t *dns = (dns_t *) arg;
	struct in_addr address;
	struct hostent *result;

	assert(dns);

	inet_aton(dns->ip, &address);

	if((result = gethostbyaddr(&address, sizeof(address), AF_INET)) == NULL)
		strncpy(dns->dns, dns->ip, sizeof(dns->dns) - 1);
	else
		strncpy(dns->dns, result->h_name, sizeof(dns->dns) - 1);

	list_push(dns_list, dns);
	pthread_exit(NULL);
}

/* public functions */
extern const char *socket_ip2host(const char *ip)
{
	pthread_t thread;
	pthread_attr_t attr;
	node_t *node = NULL;
	dns_t *dns;

	while(isspace(*ip))
		ip++;

	for(list_next(dns_list, &node); node != NULL; list_next(dns_list, &node))
	{
		dns_t *scan = (dns_t *) node->data;

		assert(scan);

		if(!strcmp(ip, scan->ip))
			return(scan->dns);
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if((dns = malloc(sizeof(dns_t))) == NULL)
	{
		fprintf(stderr, "malloc: %.100s (%i)\n", strerror(errno), errno);
		return(ip);
	}

	strncpy(dns->ip, ip, sizeof(dns->ip) - 1);

	if(pthread_create(&thread, &attr, _socket_ip2host, dns) != 0)
	{
		fprintf(stderr, "pthread_create: %.100s (%i)\n", strerror(errno), errno);
		list_delete_data(dns_list, dns);
		return(ip);
	}

	return(ip);
}

extern size_t socket_read(int fd, void *buf, size_t count)
{
	size_t n;

	assert(buf);

	socket_socket_read_again:
	n = read(fd, buf, count);

	if(n == -1)
	{
		if(errno == EINTR || errno == EAGAIN)
		{
			goto socket_socket_read_again;
		}
		else
		{
			fprintf(stderr, "read: %.100s (%i)\n", strerror(errno), errno);
			return(n);
		}
	}

	return(n);
}

extern size_t socket_write(socket_t *s, const char *buf)
{
	if(buf == NULL)
		return(-1);

	return _socket_write(s, buf, strlen(buf));
}

extern size_t socket_prompt(socket_t *s, const char *buf)
{
	char *ptr;
	int n;

	assert(buf);

	if((ptr = malloc(strlen(buf) + 3)) == NULL)
	{
		fprintf(stderr, "malloc(%i): %.100s (%i)\n", strlen(buf) + 3, strerror(errno), errno);
		return(-1);
	}

	snprintf(ptr, strlen(buf) + 2, "%s", buf);

	n = _socket_write(s, ptr, strlen(ptr));
	free(ptr);
	return(n);
}

extern size_t socket_writef(socket_t *s, const char *format, ...)
{
	va_list args;
	char *ref;
	size_t n;

	assert(format);

	if((ref = malloc(1024)) == NULL)
	{
		fprintf(stderr, "malloc(1024): %.100s (%i)\n", strerror(errno), errno);
		return(-1);
	}


	memset(ref, '\0', 1024);
	va_start(args, format);
	vsnprintf(ref, 1023, format, args);
	va_end(args);

	n = _socket_write(s, ref, strlen(ref));

	free(ref);
	return(n);
}

extern int socket_read_print(void *arg)
{
	player_t *p = (player_t *) arg;
	socket_t *s;

	assert(p);
	s = &p->socket;
	assert(s);

	fprintf(stderr, "pthread(%ld): %.63s: %.100s", (unsigned long int) pthread_self(), __func__, s->buf);
	return(0);
}

extern socket_t *socket_new(void)
{
        socket_t *s;

        if((s = calloc(1, sizeof(socket_t))) == NULL)
        {
                fprintf(stderr, "calloc(%i): %.100s (%i)\n", sizeof(socket_t), strerror(errno), errno);
                return(NULL);
        }

	assert(s);
        memset(s, '\0', sizeof(s));

	s->flags = 0;
	s->port = 0;
	s->fd = 0;
	s->len = 0;
	s->addrlen = 0;
	s->pin.bytes = s->pin.errors = s->pin.dropped = s->pin.packets = 0;
	s->pout.bytes = s->pout.errors = s->pout.dropped = s->pout.packets = 0;

	memset(s->ip, '\0', sizeof(s->ip));
	memset(s->buf, '\0', sizeof(s->buf));
	memset(s->name, '\0', sizeof(s->name));

	s->list = NULL;
	s->node = NULL;
	s->f_loop = NULL;
	s->f_connect = NULL;
	s->f_disconnect = NULL;
	
        return(s);
}

extern void *socket_select(void *arg)
{
	player_t *p = (player_t *) arg;
	socket_t *s;

	if(pthread_detach(pthread_self()))
	{
		fprintf(stderr, "pthread_detach(%lu): %.100s (%i)\n", (unsigned long int) pthread_self(), strerror(errno), errno);
		player_close(p);
		pthread_exit(NULL);
	}

	signal(SIGPIPE, SIG_IGN);

	assert(p);
	s = &p->socket;
	assert(s);

	login_welcome(p, NULL, 0);

	for( ; ; )
	{
		FD_ZERO(&s->client_fds);
		FD_SET(s->fd, &s->client_fds);

		pthread_mutex_lock(&s->lock);
		s->tv.tv_sec = 1;
		s->tv.tv_usec = 0;
		p->oflags = p->flags;
		pthread_mutex_unlock(&s->lock);

		if(p->flags & LOGOUT)
			break;

		if(select(FD_SETSIZE, &s->client_fds, NULL, NULL, &s->tv) == -1)
		{
			if(errno == EINTR)
				continue;

			fprintf(stderr, "select: %.100s (%i)\n", strerror(errno), errno);
			player_close(p);
			pthread_exit(NULL);
		}

		if(FD_ISSET(s->fd, &s->client_fds))
		{
			if(p->socket.f_read)
				p->socket.f_read(p, s->buf, s->len);

			if(p->flags & P_READY)
			{
				if(p->f_mode) p->f_mode(p, p->ibuffer, p->ibuffer_ptr);
				memset(p->ibuffer, '\0', sizeof(p->ibuffer));
				p->ibuffer_ptr = 0;
				socket_write(&p->socket, p->prompt);
				fprintf(stderr, "FD SET\n");
			}
		}
	} /* for( ; ; ) */

	player_close(p);
	pthread_exit(NULL);
}

extern void *socket_pthread_per_accept(void *arg)
{
	socket_t *listener = (socket_t *) arg;
	player_t *new_player = NULL;
	int connfd = -1;
	socklen_t clilen;

	assert(listener);
	assert(listener->list);
	assert(listener->f_connect);

	list_push(socket_list, listener);

	for( ; ; )
	{
		if((new_player = player_new()) == NULL)
		{
			socket_close(listener);
			pthread_exit(NULL);
		}

		new_player->plist = listener->list;
		new_player->socket.f_read = process_input;
		new_player->f_mode = login_get_name;

		clilen = sizeof(new_player->socket.in);

socket_pthread_per_accept_again:
		if((connfd = accept(listener->fd, (struct sockaddr *) &new_player->socket.in, &clilen)) == -1)
		{
#ifdef EPROTO
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EPROTO || errno == ECONNABORTED)
#else
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED)
#endif
			{
				goto socket_pthread_per_accept_again;
			}

			fprintf(stderr, "accept: %.100s (%i)\n", strerror(errno), errno);
			socket_close(listener);
			player_close(new_player);
			pthread_exit(NULL);
		}

		strncpy(new_player->socket.ip, inet_ntoa(new_player->socket.in.sin_addr), sizeof(new_player->socket.ip) - 1);
		fprintf(stderr, "IP = %s\n", new_player->socket.ip);
		fprintf(stderr, "DNS = %s\n", socket_ip2host(new_player->socket.ip));

		new_player->socket.fd = connfd;

		list_push(listener->list, new_player);
		list_push(player_list, new_player);

		if(pthread_create(&new_player->thread, NULL, listener->f_connect, new_player) != 0)
		{
			fprintf(stderr, "pthread_create: %.100s (%i)\n", strerror(errno), errno);
			player_close(new_player);
			continue;
		}
	} /* for( ; ; ) */

	fprintf(stderr, "debug: flags: shutdown.\n");
	socket_close(listener);
	pthread_exit(NULL);
}

