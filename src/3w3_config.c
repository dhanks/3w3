#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "config.h"
#include "3w3_config.h"

/* create structure for configuration file */

typedef enum
{
	eBadOption,
	ePort,
	eListenAddress,
	eLogFile,
	eLogLevel,
	eUser,
	eKeepAlive
} ConfigKey;

static struct
{
	const char *name;
	ConfigKey key;
} keyword[] =
{
	{ "port", ePort },
	{ NULL, eBadOption }
};

/* static functions */

static ConfigKey get_token(const char *key)
{
	register int i;

	for(i = 0; keyword[i].name != NULL; i++)
		if(!strcasecmp(key, keyword[i].name))
			return keyword[i].key;

	return eBadOption;
}

/* public functions */

void init_option(struct ConfigOption *o)
{
	strncpy(o->name, "3w3", 3);
	strncpy(o->logfile, "./3w3.log", strlen("./3w3.log"));
	strncpy(o->logfile, "./3w3.conf", strlen("./3w3.conf"));
	o->port = 2010;
	o->level = eINFO;
	o->uid = geteuid();
	o->gid = getegid();
}

int load_option(struct ConfigOption *o)
{
	FILE *f;
	char line[BUFSIZ];
	char file[BUFSIZ];
	int ln = 1;
	struct passwd *p;

	strncpy(file, o->configfile, sizeof(file) - 1);

	if(file == NULL)
	{
		fprintf(stderr, "3w3: please supply a configuration file.\n");
		return(-1);
	}

	if((f = fopen(file, "r")) == NULL)
	{
		fprintf(stderr, "3w3: %.100s: %.100s (%i)\n", file, strerror(errno), errno);
		return(-1);
	}

	for(ln = 1; fgets(line, sizeof(line), f); ln++)
	{
		char key[BUFSIZ];
		char val[BUFSIZ];
		char *ptr;
		int x = 0;

		ptr = line;

		/* skip initial whitspace */
		while(isspace(*ptr))
			ptr++;

		/* skip blank lines */
		if(strlen(ptr) <= 0)
			continue;

		/* skip comments and newlines */
		if(*ptr == '#' || *ptr == '\n' || *ptr == '\r')
			continue;

		switch(sscanf(ptr, "%1000[^ \t]%1000s\n", (char *)&key, (char *)&val))
		{
			/* found both a key and a value */
			case 2:
				switch(get_token(key))
				{
					case eBadOption:
						fprintf(stderr, "[%.100s, line %i]: bad configuration option `%.63s'\n", file, ln, key);
						return(-1);
						break;
				}

				break;
			case 1:
				if(key[strlen(key) - 1] == '\n')
					key[strlen(key) - 1] = '\0';

				switch(get_token(key))
				{
					case eBadOption:
						fprintf(stderr, "[%.100s, line %i]: bad configuration option `%.63s'\n", file, ln, key);
						return(-1);
						break;
					default:
						fprintf(stderr, "[%.100s, line %i]: missing arguement\n", file, ln);
						return(-1);
						break;
				}

				break;
			default:
				fprintf(stderr, "[%.100s, line %i]: invalid syntax.\n", file, ln);
				return(-1);
				break;
		}
	}

	fclose(f);

	return(0);
}
