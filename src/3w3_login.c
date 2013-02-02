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
#include <ctype.h>
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
#include "3w3_process_input.h"
#include "3w3_globals.h"

/* ======================================================================= */
/* private functions declarations */
/* ======================================================================= */

/* ======================================================================= */
/* public functions */
/* ======================================================================= */

extern int login_get_name(void *arg, const char *input, size_t length)
{
	player_t *p = (player_t *) arg;
	socket_t *s;
	const char *c;

	assert(p);
	s = &p->socket;

	if(length <= 0)
	{
		sleep(1);
		socket_prompt(s, "Please enter your name: ");
		return(-1);
	}

	if(length > 32)
	{
		socket_write(s, "Username too long.\n");
		sleep(2);
		socket_prompt(s, "Please enter your name: ");
		return(-1);
	}

	for(c = input; *c; c++)
	{
		if(!isalpha(*c))
		{
			socket_write(s, "Usernames may only contain alphabetic characters.\n");
			sleep(2);
			socket_prompt(s, "Please enter your name: ");
			return(-1);
		}
	}

	strncpy(p->name, input, length);
	p->f_mode = process_command;
	p->flags |= LOGGED_IN;
	return(0);
}

extern int login_welcome(void *arg, const char *input, size_t length)
{
	player_t *p = (player_t *) arg;
	socket_t *s;

	assert(p);
	s = &p->socket;

	socket_write(s, dfile_get(DFILE_ISSUE));
	socket_prompt(s, "Please enter your name: ");

	return(0);
}
