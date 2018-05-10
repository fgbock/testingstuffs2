//#ifndef DROPBOXUTILS_HEADER
//#define DROPBOXUTILS_HEADER
#include"dropboxUtils.h"


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

int create_home_dir(char *userID){
	char *dir = malloc((strlen(userID)+10)*sizeof(char));	
	strcpy(dir, "mkdir ~/");
	strcat(dir, userID);
	int ret = system(dir);
	free(dir);
	return ret;
}


//#endif
