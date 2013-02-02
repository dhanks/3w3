#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

#include "config.h"
#include "../sqlite/sqlite3.h"
#include "3w3_list.h"
#include "3w3_socket.h"
#include "3w3_afunix.h"

int afunix_udp_init(socket_t *udp, const char *name)
{
	assert(udp);

	if((udp->fd = socket(AF_LOCAL, SOCK_STREAM, IPPROTO_IP)) == -1)
	{
		fprintf(stderr, "socket: %.100s (%i)\n", strerror(errno), errno);
		return(-1);
	}

	if(unlink(name) == -1)
	{
		switch(errno)
		{
			case ENOENT:
				break;
			default:
				fprintf(stderr, "unlink: %.100s: %.100s (%i)\n", name, strerror(errno), errno);
				return(-1);
				break;
		}
	}

	memset(&udp->udp, '\0', sizeof(udp->udp));
	udp->udp.sun_family = AF_LOCAL;
	strncpy(udp->udp.sun_path, name, sizeof(udp->udp.sun_path) - 1);

	if(bind(udp->fd, &udp->udp, sizeof(udp->udp)) == -1)
	{
		fprintf(stderr, "bind: %.100s (%i)\n", strerror(errno), errno);
		return(-1);
	}

	if(listen(udp->fd, 512) == -1)
	{
		fprintf(stderr, "listen: %.100s (%i)\n", strerror(errno), errno);
		return(-1);
	}

	return(0);
}
