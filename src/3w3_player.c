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
#include <pthread.h>

#include "config.h"
#include "../sqlite/sqlite3.h"
#include "3w3_list.h"
#include "3w3_mpool.h"
#include "3w3_hash.h"
#include "3w3_socket.h"
#include "3w3_afunix.h"
#include "3w3_player.h"
#include "3w3_dfile.h"
#include "3w3_telnet.h"
#include "3w3_globals.h"

/* private functions */

/* public functions */
extern void player_close(player_t *p)
{
	assert(p);
	assert(p->plist);

	if(shutdown(p->socket.fd, SHUT_RDWR) == -1)
		fprintf(stderr, "shutdown(%i): %.100s (%i)\n", p->socket.fd, strerror(errno), errno);
	if(close(p->socket.fd) == -1)
		fprintf(stderr, "close(%i): %.100s (%i)\n", p->socket.fd, strerror(errno), errno);

	list_delete_data(p->plist, p);
	list_delete_data(player_list, p);
	free(p);
}

extern player_t *player_new(void)
{
        player_t *p;

        if((p = calloc(1, sizeof(player_t))) == NULL)
        {
                fprintf(stderr, "calloc(%i): %.100s (%i)\n", sizeof(player_t), strerror(errno), errno);
                return(NULL);
        }

	assert(p);
        memset(p, '\0', sizeof(player_t));

	memset(p->ibuffer, '\0', sizeof(p->ibuffer));
	p->ibuffer_ptr = 0;

	memset(p->name, '\0', sizeof(p->name));
	memset(p->prompt, '\0', sizeof(p->prompt));

	strncpy(p->prompt, "$ ", 2);

	p->socket.telnet = TEL_GA | TEL_EOR;
	p->socket.telnet &= ~TEL_PROMPT;
        return(p);
}
