#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/telnet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
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
/* private functions declarations */
/* ======================================================================= */

/* ======================================================================= */
/* public functions */
/* ======================================================================= */

extern int process_command(void *arg, const char *input, size_t length)
{
	player_t *p = (player_t *) arg;
	socket_t *s;
	command_t *command;

	if(length <= 0)
		return(-1);

	assert(p);
	s = &p->socket;

	if(hash_find(command_hash, (void *) input, length, &command) == 0)
		return command->command(p, input, length);
	else
		socket_writef(s, "%.63s: command not found\n", input);

	return(-1);
}

extern int process_input(void *arg, const char *input, size_t length)
{
	player_t *p = (player_t *) arg;
	socket_t *s;
	unsigned char c;
	int size;

	assert(p);
	s = &p->socket;

	if(ioctl(s->fd, FIONREAD, &size) == -1 || size <= 0)
	{
		fprintf(stderr, "ioctl: %.100s (%i)\n", strerror(errno), errno);
		player_close(p);
		pthread_exit(NULL);
		return(-1);
	}

	for( ; size; size--)
	{
		if(socket_read(s->fd, &c, 1) <= 0)
		{
			player_close(p);
			pthread_exit(NULL);
			return(-1);
		}

		fprintf(stderr, "size = %i\n", size);

		switch(c)
		{
			case DO:
			case DONT:
			case WONT:
			case WILL:
			case SB:
			case SE:
				fprintf(stderr, "OTHER\n");
				break;

			case BREAK:
			case IP:
				player_close(p);
				pthread_exit(NULL);
				break;
			case IAC:
				fprintf(stderr, "IAC\n");
				if(socket_read(s->fd, &c, 1) <= 0)
				{
					player_close(p);
					pthread_exit(NULL);
					return(-1);
				}
				size--;
				fprintf(stderr, "IAC after\n");

				switch(c)
				{
					case AYT:
						socket_write(s, "[3w3]: yes\n");
						break;
					case IP:
						player_close(p);
						pthread_exit(NULL);
						break;
					case EC:
						if(p->ibuffer_ptr)
							p->ibuffer_ptr--;

						p->ibuffer[p->ibuffer_ptr] = '\0';
						socket_writef(s, "%c%c%c", (char) IAC, (char) DO, (char) EC);
						break;
					case EL:
						memset(p->ibuffer, '\0', sizeof(p->ibuffer));
						p->ibuffer_ptr = 0;
						break;
				}
				break;
			case '\n':
				if(!(p->flags & P_LAST_R))
					p->flags |= (P_READY|P_LAST_N);
				fprintf(stderr, "READY\n");
				break;
			case '\r':
				if(!(p->flags & P_LAST_N))
					p->flags |= (P_READY|P_LAST_R);
				fprintf(stderr, "READY\n");
				break;
			default:
				p->flags &= ~(P_LAST_N|P_LAST_R);

				if(c == 8 || c == 127)
				{
					if(p->ibuffer_ptr)
						p->ibuffer_ptr--;

					p->ibuffer[p->ibuffer_ptr] = '\0';
					socket_writef(s, "%c%c%c", (char) IAC, (char) DO, (char) EC);
				}

				if(c > 31 && c < 127)
				{
					if(p->ibuffer_ptr > sizeof(p->ibuffer) - 1)
					{
						socket_write(s, "[3w3]: input truncated.\n");
						memset(p->ibuffer, '\0', sizeof(p->ibuffer));
						p->ibuffer_ptr = 0;
					}
					else
					{
						p->ibuffer[p->ibuffer_ptr++] = c;
					}
				}
				break;
		}
	}

	return(0);
}
