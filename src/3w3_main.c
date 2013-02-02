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
#include <signal.h>
#include <assert.h>

#include "config.h"
#include "../sqlite/sqlite3.h"
#include "3w3_list.h"
#include "3w3_socket.h"
#include "3w3_player.h"
#include "3w3_afunix.h"
#include "3w3_afinet.h"
#include "3w3_logger.h"
#include "3w3_interface.h"
#include "3w3_config.h"
#include "3w3_mpool.h"
#include "3w3_hash.h"
#include "3w3_dfile.h"
#include "3w3_command.h"
#include "3w3_globals.h"

int main(int argc, char *argv[])
{
	int c;
	struct ConfigOption option;

	strncpy(option.configfile, "./3w3.conf", sizeof(option.configfile) - 1);

	while((c = getopt(argc, argv, "c:?hvV")) != EOF)
	{
		switch(c)
		{
			case 'c':
				strncpy(option.configfile, optarg, sizeof(option.configfile) - 1);
				break;
			case 'h':
			case '?':
				fprintf(stdout, "Usage: 3w3 [ -c config_file ] [-hvV]\n");
				fprintf(stdout, "3w3 Enterprise Chat Server\n");
				fprintf(stdout, " -c\tspecify 3w3.conf file.\n");
				fprintf(stdout, " -h\tdisplay this help synopsis.\n");
				fprintf(stdout, " -v\tdisplay version information.\n");
				exit(EXIT_SUCCESS);
				break;
			case 'v':
			case 'V':
				fprintf(stdout, "3w3 Enterprise Chat Server version information:\n");
				fprintf(stdout, " + %.63s version %.63s\n\n", PACKAGE_NAME, PACKAGE_VERSION);
				fprintf(stdout, "Library information:\n");
				fprintf(stdout, " + SQLite version %.31s\n", SQLITE_VERSION);
				exit(EXIT_SUCCESS);
			default:
				fprintf(stderr, "Try `3w3 -h' for more information.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	signal(SIGPIPE, SIG_IGN);

	init_option(&option);

#if 0
	if(load_option(&option) < 0)
		exit(EXIT_FAILURE);
#endif

	assert(*(argv+2));

	if((player_list = list_new()) == NULL)
		exit(EXIT_FAILURE);
	if((socket_list = list_new()) == NULL)
		exit(EXIT_FAILURE);
	if((interface_list = list_new()) == NULL)
		exit(EXIT_FAILURE);
	if((dns_list = list_new()) == NULL)
		exit(EXIT_FAILURE);

	if((command_hash = hash_new(1, 1, 0)) == NULL)
		exit(EXIT_FAILURE);
	if(dfile_new(DFILE_ISSUE, "/home/dhanks/3w3/lib/banners/issue") == -1)
		exit(EXIT_FAILURE);

	fprintf(stderr, "command_init = %i\n", command_init());

	if(interface_tcp_create(socket_pthread_per_accept, socket_select, *(argv+1), *(argv+2)) == -1)
		exit(EXIT_FAILURE);
	if(interface_tcp_create(socket_pthread_per_accept, socket_select, *(argv+1), "6666") == -1)
		exit(EXIT_FAILURE);
	if(interface_tcp_create(socket_pthread_per_accept, socket_select, *(argv+1), "7777") == -1)
		exit(EXIT_FAILURE);

	for( ; ; )
	{
		sleep(2);
	}

	exit(EXIT_SUCCESS);
}
