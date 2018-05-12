#ifndef DROPBOXSERVER_C
#define DROPBOXSERVER_C

#define SOCKET int
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "dropboxUtils.h"

#define MAIN_PORT 6000

struct file_info {
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
};

struct client {
  int devices[2];
  struct sockaddr sender;
  char client_buffer[1250];
  char userid [MAXNAME];
  struct file_info info [MAXFILES];
  int logged_in;
};

void sync_server(){

}

void send_file(char *file){

}

void *session_manager(void *args){
	struct sockaddr_in session;
	struct client *session_client;
	session_client = args;
	printf("\n\nNew session created...\n");
	printf("User ID is: %s\n", session_client->userid);
	return 0;
}

int main(int argc,char *argv[]){
	struct sockaddr_in server;
	struct sockaddr client;
	SOCKET s_socket;
	int server_len, received, i, online = 1, client_len = sizeof(struct sockaddr_in), session_counter = 0;
	char packet_buffer[1250];
	char op_code[6];
	char aux_username[20];

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
		  printf("Binding error\n");
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
		received = recvfrom(s_socket,packet_buffer,sizeof(packet_buffer),0,(struct sockaddr *) &client,(socklen_t *)&client_len);
		if (!received){
			printf("ERROR: Package reception error.\n\n");
		}
		for (i = 0; i < 4; i++){
			op_code[i] = packet_buffer[i];
		}
		if (strcmp(op_code,"logins")){
			for (i = 0; i < 20; i++){
				aux_username[i] = packet_buffer[10+i];
			}
			session_counter++;
			sendto(s_socket,"ACK",sizeof("ACK"),0,(struct sockaddr *)&client, client_len);
			pthread_t tid;
			struct client new_client;
			strcpy(new_client.userid, aux_username);
			new_client.sender = client;
			pthread_create(&tid, NULL, session_manager, &new_client);
		}
		else{
			printf("Redirecting packet to session manager...");
		}
	}
	return 0;
}


#endif
