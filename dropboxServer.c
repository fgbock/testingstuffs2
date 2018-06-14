#ifndef DROPBOXSERVER_C
#define DROPBOXSERVER_C

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include "dropboxUtils.h"
#include <dirent.h>
#include <netdb.h>
#include <pwd.h>
#include <unistd.h>

#define SOCKET int
#define PACKETSIZE 1250
#define MAIN_PORT 6000
#define MAXCLIENTS 10
#define MAXSESSIONS 2
#define NACK 0
#define ACK 1
#define UPLOAD 2
#define DOWNLOAD 3
#define DELETE 4
#define LIST 5
#define CLOSE 6
#define LOGIN 7


// Structures
struct file_info {
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
};

struct client {
	char user_id [MAXNAME];
	int session_active [MAXSESSIONS];
	short int session_port [MAXSESSIONS];
	//
	SOCKET socket;
	struct file_info info [MAXFILES];
};

struct packet {
	short int opcode;
	short int seqnum;
	char data [PACKETSIZE - 4];
};

struct pair {
	int c_id;
	int s_id;
};

// Global Variables
struct client client_list [MAXCLIENTS];

// Subroutines
int inactive_client(int index){
	int i;
	for (i = 0; i < MAXSESSIONS; i++){
		if (client_list[index].session_active[i] == 1) return 0;
	}
	return 1;
}

int identify_client(char user_id [MAXNAME], int* client_index){
	int i;
	*client_index = -1;
	for (i = 0; i < MAXCLIENTS; i++){
		if (strcmp(client_list[i].user_id,user_id) == 0){
			*client_index = i;
			return 0;
		}
	}
	for (i = 0; i < MAXCLIENTS; i++){
		if (inactive_client(i)){
			strncpy(client_list[i].user_id, user_id, MAXNAME);
			*client_index = i;
			return 0;
		}
	}
	return -1;
}

void send_file(char *file, int socket, char *userID, struct sockaddr client_addr){
	char path[256];
	strcpy(path, "~/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);
	printf("File path is :%s\n",path);
	send_file_to(socket, path, client_addr);
}

void receive_file(char *file, int socket, char*userID){
	char path[256];
	strcpy(path, "~/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);
	printf("File path is :%s\n",path);
	receive_file_from(socket, path, client_addr);
}

int delete_file(char *file, int socket, char*userID){
	if(remove(file) == 0){
		return 1;
	}
	return 0;
}

void list_files(SOCKET socket, struct sockaddr client){
	DIR *dp;
	struct dirent *ep;
	char files_list[110];
	struct packet reply;

	reply.opcode = LIST;
	dp = opendir ("./");
	if (dp != NULL){
			while (ep = readdir (dp)){
				strcat(files_list,ep->d_name);
			}
			strncpy(reply.data, files_list, 110);
			sendto(socket, (char *) &reply, PACKETSIZE,0,(struct sockaddr *)&client, sizeof(client));
			(void) closedir (dp);
	}
	else perror ("Couldn't open the directory");
}

void *session_manager(void* args){
	char filename[MAXNAME];
	SOCKET session_socket;
	struct sockaddr client;
	struct sockaddr_in session;
	struct packet request, reply;
	int c_id, s_id, session_port, session_len, client_len = sizeof(struct sockaddr_in), active = 1;

	// Getting thread arguments
	struct pair *session_info = (struct pair *) args;
	c_id = (*session_info).c_id;
	s_id = (*session_info).s_id;
	printf("\nClient Id is %d and  Server Id is %d\n\n", c_id, s_id);
	session_port = (int) client_list[c_id].session_port[s_id];


	// Set up a new socket
	if((session_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Socket creation failure.\n");
		exit(1);
	}
	memset((void *) &session,0,sizeof(struct sockaddr_in));
	session.sin_family = AF_INET;
	session.sin_addr.s_addr = htonl(INADDR_ANY);
	session.sin_port = htons(session_port);
	session_len = sizeof(session);
	if (bind(session_socket,(struct sockaddr *) &session, session_len)) {
		printf("Binding error\n");
		active = 0;
	}
	else{
		printf("Socket initialized, waiting for requests.\n\n");
	}
	// Setup done

	while(active){
		if (!recvfrom(session_socket, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &client, (socklen_t *) &client_len)){
			printf("ERROR: Package reception error.\n\n");
		}
		switch(request.opcode){
			case UPLOAD:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				strncpy(filename, request.data, MAXNAME);
				//receive_file(filename, session_socket, client_list[c_id].user_id);
				break;
			case DOWNLOAD:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				strncpy(filename, request.data, MAXNAME);
				//send_file(filename, session_socket, client_list[c_id].user_id, client);
				break;
			case LIST:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				list_files(session_socket, client);
				break;
			case DELETE:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				strncpy(filename, request.data, MAXNAME);
				delete_file(filename, session_socket, client_list[c_id].user_id);
				break;
			case CLOSE:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				client_list[c_id].session_active[s_id] = 0;
				pthread_exit(); // Should have an 'ack' by the client allowing us to terminate, ideally!
				break;
			default:
				printf("ERROR: Invalid packet detected.\n\n");
		}
		//
	}
}

int login(struct packet login_request){
	struct pair *thread_param;
	char user_id [MAXNAME];
	int i, index;
	short int port;
	pthread_t tid;


	strncpy (user_id, login_request.data, MAXNAME);
	identify_client(user_id, &index);
	if (login_request.opcode != LOGIN || index == -1){
		return -1;
	}
	for (i = 0; i < MAXSESSIONS; i++){
		if (client_list[index].session_active[i] == 0){
			port = MAIN_PORT + (2*index) + i + 1;
			client_list[index].session_active[i] = 1;
			client_list[index].session_port[i] = (short) port;
			thread_param = malloc(sizeof(struct pair));
			(*thread_param).c_id = index;
			(*thread_param).s_id = i;
			create_server_userdir(client_list[index].user_id);
			printf("\nClient Id is %d and  Server Id is %d\n\n", index, i);
			pthread_create(&tid, NULL, session_manager, (void *) thread_param);
			return port;
		}
	}
	return -1;
}

int main(int argc,char *argv[]){
	SOCKET main_socket;
	struct sockaddr client;
	struct sockaddr_in server;
	struct packet login_request, login_reply;
	int  session_port, server_len, client_len = sizeof(struct sockaddr_in), online = 1;

	create_server_root();
	// Socket setup
	if((main_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Socket creation failure.\n");
		exit(1);
	}
	memset((void *) &server,0,sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(MAIN_PORT);
	server_len = sizeof(server);
	if (bind(main_socket,(struct sockaddr *) &server, server_len)) {
		printf("Binding error\n");
		exit(1);
	}
	printf("Socket initialized, waiting for requests.\n\n");
	// Setup done

	while(online){
		if (!recvfrom(main_socket, (char *) &login_request, PACKETSIZE, 0, (struct sockaddr *) &client, (socklen_t *) &client_len)){
			printf("ERROR: Package reception error.\n\n");
		}
		else{
			session_port = login(login_request);
			printf("\nopcode is %hi\n\n",login_request.opcode);
			printf("\ndata is %s\n\n",login_request.data);
			if (session_port > 0){
				login_reply.opcode = ACK;
				login_reply.seqnum = (short) session_port;
				sendto(main_socket, (char *) &login_reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				printf("Login succesful...\n");
			}
			else{
				login_reply.opcode = NACK;
				sendto(main_socket, (char *) &login_reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				printf("ERROR: Login unsuccesful...\n");
			}
			//
		}
		login_request.opcode = 0;
	}

	return 0;
}

#endif
