#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

#include "../sqlite/sqlite3.h"
#include "config.h"
#include "3w3_list.h"
#include "3w3_mpool.h"
#include "3w3_hash.h"
#include "3w3_socket.h"
#include "3w3_player.h"
#include "3w3_afinet.h"
#include "3w3_dfile.h"
#include "3w3_globals.h"

/* public functions */
int interface_tcp_create(void *(*f_accept)(void *), void *(*f_connect)(void *), const char *host, const char *serv)
{
	socket_t *new;

	assert(f_accept);
	assert(f_connect);
	assert(serv);

	if((new = socket_new()) == NULL)
		return(-1);

	assert(new);

	if(afinet_tcp_listen(new, host, serv) == -1)
	{
		if(new) free(new);
		return(-1);
	}

	if((new->list = list_new()) == NULL)
	{
		if(new) free(new);
		return(-1);
	}

	assert(new->list);

	new->f_accept = f_accept;
	new->f_connect = f_connect;

	list_push(interface_list, new);

	if(pthread_create(&new->thread, NULL, new->f_accept, new))
	{
		list_destroy(new->list, free);
		list_delete_data(interface_list, new);
		if(new) free(new);
		return(-1);
	}

	fprintf(stderr, "thread(%ld) %.63s: %.63s:%.32s UP\n", (unsigned long int) new->thread, __func__, host == NULL ? "ANY" : host, serv);
	return(0);
}
