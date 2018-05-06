#ifndef DROPBOXCLIENT_C
#define DROPBOXCLIENT_C

#include<stdio.h>
#include<string.h>
#include"dropboxUtils.h"

int login_server(char *host,int port){

return 0;
}
void sync_client(){



}
void send_file(char *file){


}
void get_file(char *file){



}
void delete_file(char *file){



}
void close_session(){




}

int main(int argc,char *argv[]){

	int i;
	for(i = 0;i < argc;i++){


		if(strcmp(argv[i],"-upload") == 0){

		}
		else if(strcmp(argv[i],"-download") == 0){

		}
		else if(strcmp(argv[i],"-list_server") == 0){

		}
		else if(strcmp(argv[i],"-list_client") == 0){

		}
		else if(strcmp(argv[i],"-get_sync_dir") == 0){

		}
		else if(strcmp(argv[i],"-exit") == 0){
			close_session();
		}

	}

return 0;
}


#endif
