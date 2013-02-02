#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

extern int dfile_new(dfile_type_t type, const char *path)
{
	dfile_t *new;
	int fd = -1;
	int len = 0;

	assert(path);
	assert(!(type >= DFILE_END));

	if((new = malloc(sizeof(dfile_t))) == NULL)
	{
		fprintf(stderr, "malloc(%i): %.100s (%i)\n", sizeof(dfile_t), strerror(errno), errno);
		return(-1);
	}

	memset(new->path, '\0', sizeof(new->path));
	strncpy(new->path, path, sizeof(new->path) - 1);

	new->data = NULL;
	new->size = new->flags = 0;
	new->accessed = new->reloaded = -1;
	new->loaded = new->last_accessed = new->last_reloaded = -1;

	if((fd = open(path, O_RDONLY)) == -1)
	{
		fprintf(stderr, "open(%.100s): %.100s (%i)\n", path, strerror(errno), errno);
		free(new);
		return(-1);
	}

	new->loaded = time(0);
	new->size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	if((new->data = malloc(new->size + 1)) == NULL)
	{
		fprintf(stderr, "malloc(%ld): %.100s (%i)\n", new->size + 1, strerror(errno), errno);
		close(fd);
		free(new);
		return(-1);
	}

	memset(new->data, '\0', sizeof(new->data));

	if((len = read(fd, new->data, new->size)) < new->size)
	{
		fprintf(stderr, "%.63s: %.100s: error reading after %i bytes: %.100s (%i)\n", __func__, path, len, strerror(errno), errno);
		close(fd);
		free(new);
		return(-1);
	}

	close(fd);

	*(dfile_array + type) = new;
	pthread_mutex_init(&new->lock, NULL);
	return(0);
}

extern char *dfile_get(dfile_type_t type)
{
	dfile_t *d;

	assert(!(type >= DFILE_END));

	d = *(dfile_array + type);

	if(d != NULL)
	{
		pthread_mutex_lock(&d->lock);
		d->accessed++;
		d->last_accessed = time(0);
		pthread_mutex_unlock(&d->lock);
		return d->data;
	}
	else
	{
		return(NULL);
	}
}
