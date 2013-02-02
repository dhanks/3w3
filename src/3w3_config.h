struct ConfigOption
{
	char name[BUFSIZ];
	char logfile[BUFSIZ];
	char configfile[BUFSIZ];
	int port;
	enum { eERROR, eINFO, eDEBUG1, eDEBUG2, eDEBUG3 } level;
	uid_t uid;
	gid_t gid;
};

void init_option(struct ConfigOption *);
int load_option(struct ConfigOption *);
