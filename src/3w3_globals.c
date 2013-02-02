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
#include "3w3_socket.h"
#include "3w3_player.h"
#include "3w3_afinet.h"
#include "3w3_mpool.h"
#include "3w3_hash.h"
#include "3w3_dfile.h"

/* public variables */
list_t *socket_list;
list_t *interface_list;
list_t *player_list;
list_t *dns_list;

hash_table_t *command_hash;

dfile_t *dfile_array[DFILE_END];
