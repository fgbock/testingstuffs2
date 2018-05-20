//#ifndef DROPBOXUTILS_HEADER
//#define DROPBOXUTILS_HEADER

#include<string.h>
#include<stdlib.h>


#define MAXNAME 20
#define MAXFILES 10
#define CHUNK 1240
#define OPCODE 10

int create_home_dir(char *userID);

int create_home_dir_server(char *userID);

int create_server_userdir(char *userID);

int create_server_root();

int receive_int_from(int socket);

int send_int_to(int socket, int op);

char* receive_string_from(int socket);

int send_string_to(int socket, char* str);

int receive_file_from(int socket, char* file_name);

int send_file_to(int socket, char* file_name);

char * getArgument(char command[100]);
//#endif
