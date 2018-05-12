//#ifndef DROPBOXUTILS_C
//#define DROPBOXUTILS_C

#include<string.h>
#include<stdlib.h>


#define MAXNAME 20
#define MAXFILES 10

int create_home_dir(char *userID);

int receive_int_from(int socket);

int send_int_to(int socket, int op);

char* receive_string_from(int socket);

int send_string_to(int socket, char* str);

//#endif
