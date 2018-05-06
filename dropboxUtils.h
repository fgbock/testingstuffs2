#ifndef DROPBOXUTILS_C
#define DROPBOXUTILS_C

#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>

#define MAXNAME 20
#define MAXFILES 10

int create_sync(char *userID){
	char aux_name[MAXNAME+5] = "/home/";	
	//return mkdir(strcat(aux_name,userID), 0700);
	printf("\nsdsij\n");
	return mkdir("~/123", 0777);
}

#endif
