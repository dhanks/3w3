#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <pthread.h>

#include "config.h"
#include "3w3_list.h"
#include "3w3_mpool.h"
#include "3w3_hash.h"
#include "3w3_socket.h"
#include "3w3_player.h"
#include "3w3_afunix.h"
#include "3w3_command.h"
#include "3w3_dfile.h"
#include "3w3_globals.h"


/* ======================================================================= */
/* public functions */
/* ======================================================================= */
extern int command_init(void)
{
	int n = 0;

	n += command_push("who", &command_who, 0, 0);
	n += command_push("twho", &command_twho, 0, 0);
	n += command_push("quit", &command_quit, 0, 0);
	n += command_push("exit", &command_quit, 0, 0);

	return(n);
}

extern int command_push(const char *key, int (*command) (void *, const char *, size_t), unsigned long level, unsigned long flags)
{
	command_t *new = NULL;

	assert(key);
	assert(command);

	if((new = malloc(sizeof(command_t))) == NULL)
	{
		fprintf(stderr, "malloc: %.100s (%i)\n", strerror(errno), errno);
		return(-1);
	}

	assert(new);

	new->key = key;
	new->command = command;
	new->level = level;
	new->flags = flags;

	return hash_replace(command_hash, (void *) key, strlen(key), new, NULL);
}

extern int command_who(void *arg, const char *input, size_t length)
{
	player_t *p = (player_t *) arg;
	node_t *node = NULL;
	socket_t *s;

	assert(p);
	s = &p->socket;
	assert(s);

	socket_write(s, "=======================================================================\n");
	for(list_next(player_list, &node); node != NULL; list_next(player_list, &node))
	{
		player_t *scan = (player_t  *) node->data;

		if(scan->flags & LOGGED_IN)
			socket_writef(s, "%.63s %p\n", scan->name, scan->thread);
	}
	socket_write(s, "=======================================================================\n");

	return(0);
}

extern int command_twho(void *arg, const char *input, size_t length)
{
	player_t *p = (player_t *) arg;
	socket_t *s;
	struct rusage pre_usage;
	struct rusage post_usage;
	struct timeval pre_time;
	struct timeval post_time;
	double usr, sys, real;


	assert(p);
	s = &p->socket;
	assert(s);

	gettimeofday(&pre_time, 0);
	getrusage(RUSAGE_SELF, &pre_usage);
	
	command_who(p, NULL, -1);

	getrusage(RUSAGE_SELF, &post_usage);
	gettimeofday(&post_time, 0);

	usr =	(post_usage.ru_utime.tv_sec + post_usage.ru_utime.tv_usec/1000000.0)
		-
		(pre_usage.ru_utime.tv_sec + pre_usage.ru_utime.tv_usec/1000000.0);

	sys =	(post_usage.ru_stime.tv_sec + post_usage.ru_stime.tv_usec/1000000.0)
		-
		(pre_usage.ru_stime.tv_sec + pre_usage.ru_stime.tv_usec/1000000.0);;

	real =	(post_time.tv_sec + post_time.tv_usec/1000000.0)
		-
		(pre_time.tv_sec + pre_time.tv_usec/1000000.0);

	socket_writef(s, "Usr time:  %0.05f\n", usr);
	socket_writef(s, "Sys time:  %0.05f\n", sys);
	socket_writef(s, "Real time: %0.05f\n", real);
	socket_writef(s, "Page faults: %ld\n", post_usage.ru_majflt - pre_usage.ru_majflt);
	socket_writef(s, "Page reclaims: %ld\n", post_usage.ru_minflt - pre_usage.ru_minflt);
	socket_writef(s, "Swaps: %ld\n", post_usage.ru_nswap - pre_usage.ru_nswap);
	return(0);
}

extern int command_quit(void *arg, const char *input, size_t length)
{
	player_t *p = (player_t *) arg;
	socket_t *s;

	assert(p);
	s = &p->socket;
	assert(s);

	player_close(p);
	pthread_exit(NULL);
	return(0);
}
