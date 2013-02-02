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

#include "config.h"
#include "../sqlite/sqlite3.h"
#include "3w3_list.h"
#include "3w3_socket.h"
#include "3w3_afunix.h"

#define LOG "/tmp/3w3.log"

/* public functions */
void *logger_init(void *arg)
{
	socket_t **logger = arg;

#if 0
	if(pthread_detach(pthread_self()) != 0)
	{
		fprintf(stderr, "pthread_deteach: %.100s (%i)\n", strerror(errno), errno);
		return(NULL);
	}
#endif

	if(afunix_udp_init(*logger, "/tmp/.3w3-logger"))
		return(NULL);

	if(((*logger)->list = list_new()) == NULL)
		return(NULL);

	(*logger)->f_loop = socket_pthread_per_accept;
	(*logger)->f_connect = socket_select;
	(*logger)->f_loop(*logger);

	return(NULL);
}
