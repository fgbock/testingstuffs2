#ifndef DROPBOXSERVER_C
#define DROPBOXSERVER_C

#define SOCKET int
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include "dropboxUtils.h"

#define MAIN_PORT 6000

struct client {
  int devices[2];
  struct sockaddr sender;
  char userid[MAXNAME];
  struct file_info[MAXFILES];
  int logged_in;
}

struct file_info {
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
}

void sync_server(){

}

void receive_file(char *file){

}

void send_file(char *file){

}

int session_manager(){
	return 0;
}

int main(int argc,char *argv[]){
	struct sockaddr_in server;
	struct sockaddr client;
	SOCKET s_socket;
	int server_len, received, i, online = 1, client_len = sizeof(struct sockaddr_in);
	char packet_buffer[1250];

	if ((s_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Socket creation failure.\n");
		exit(1);
	}
/*	
	Configures domain, IP (set to any) and port (set to MAIN_PORT)
	Binds s_socket to the client sockaddr_in structure
*/
	 memset((void *) &server,0,sizeof(struct sockaddr_in));
	 server.sin_family = AF_INET;
	 server.sin_addr.s_addr = htonl(INADDR_ANY);
	 server.sin_port = htons(MAIN_PORT); 
	 server_len = sizeof(server);
	 if(bind(s_socket,(struct sockaddr *) &server, server_len)) {
		  printf("Erro no bind\n");
		  exit(1);
	 }
	printf("Socket initialized, waiting for requests.\n\n");
/*
	Main loop - receives packages and either 
		1) redirects them to a session thread, where:
			- it updates the folder (receive_file or sync are called)
			- it sends a file (sync_client or send_file are called)
		2) uses their info to create a new session thread
*/
	while(online){
		received = recvfrom(s_socket,packet_buffer,sizeof(packlet_buffer),0,(struct sockaddr *) &client,(socklen_t *)&client_len);
		if (!received){
			printf("ERROR: Package reception error.");
			exit(2);
		}
		struct client client_structure;
		client_structure.sender = client;
	}
	return 0;
}


#endif
