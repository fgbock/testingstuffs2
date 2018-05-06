#ifndef DROPBOXUTILS_HEADER
#define DROPBOXUTILS_HEADER


#define MAXNAME 20
#define MAXFILES 10

struct file_info{
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
};

struct client{
	int devices[2];
	char userid[MAXNAME];
	struct file_info fi[MAXFILES];
	int logged_in;
};

#endif
