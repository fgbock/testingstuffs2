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

// Structures
struct file_info {
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modified[MAXNAME];
  int size;
};

struct client {
  int device[2];
  char client_buffer[1250];
  char userid [MAXNAME];
  struct file_info info [MAXFILES];
  int logged_in;
};

struct session {
	int client_id;
	struct sockaddr client_address;
};

// Global Variables
struct client client_list[10];
int client_active[10];
int session_counter = 0;

// Subroutines
void sync_server(){

}

void send_file(char *file){

}

void *session_manager(void *args){
	struct session *client_session;
	client_session = (struct session *) args;
	printf("Current Client ID is: %d\n\n", client_session->client_id);
	return 0;
}

int login(struct sockaddr client, char packet_buffer[1250]){
 	char aux_username[20];
	struct client new_client;
	struct session new_session;
	pthread_t tid;
	int i, not_done = 1;
	// Verify client list
	for (i = 0; i < 20; i++){
		aux_username[i] = packet_buffer[10+i];
	}
	create_home_dir(aux_username);
	for(i = 0; i < 10; i++){
		if(strcmp(client_list[i].userid,aux_username) == 0){
			if(client_list[i].logged_in == 0){
				return 0;
			}
			else{			
				client_list[i].logged_in--; // Update logged_in
				new_session.client_id = i; 
				new_session.client_address = client;
				printf("New device added to existing session:\n");
				pthread_create(&tid, NULL, session_manager, &new_session);	
				return 1;
			}
		}
	}
	i = 0;
	while(not_done){
		if(client_active[i] == 0){
			strcpy(new_client.userid, aux_username); // Set userid
			new_client.logged_in = 1; // Set logged_in
			client_list[i] = new_client; // Place new client into the client list
			new_session.client_id = i; // Inform the session of the client's index in the list
			new_session.client_address = client;
			printf("New session created:\n");
			pthread_create(&tid, NULL, session_manager, &new_session);
			not_done = 0;
		}
		i++;
	}
	return 1;
}

int main(int argc,char *argv[]){
	struct sockaddr_in server;
	struct sockaddr client;
	SOCKET s_socket;
	int server_len, received, i, online = 1, client_len = sizeof(struct sockaddr_in), login_value;
	char packet_buffer[1250];
	char op_code[7];

	// Initializing client list (temporary measure - until we get a user list file)
	for(i = 0; i < 10; i++){
		struct client new_client;
		client_list[i] = new_client;
		strcpy(client_list[i].userid," ");
		client_active[i] = 0;
	}

	if((s_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
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
		for(i = 0; i < 6; i++){
			op_code[i] = packet_buffer[i];
		}
		if (strcmp(op_code,"logins") == 0){
			if (login(client,packet_buffer)){
				sendto(s_socket,"ACK",sizeof("ACK"),0,(struct sockaddr *)&client, client_len);
				printf("Login succesful...\n");				
			}
			else{
				sendto(s_socket,"NACK",sizeof("NACK"),0,(struct sockaddr *)&client, client_len);
				printf("ERROR: Login unsuccesful...\n");
			}
		}
		else{
			printf("Redirecting packet to session manager...");
		}
		memset(packet_buffer,0,1250);
		memset(op_code,0,7);
	}
	return 0;
}


#endif
