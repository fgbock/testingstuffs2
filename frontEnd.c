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
#define FILEPKT 8
#define LASTPKT 9
// Part II
#define PING 10
#define MAXSERVERS 3
#define FRONTEND 999
#define NONE 1000

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
	int socket_set[MAXSESSIONS];
	SOCKET socket[MAXSESSIONS];
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

struct serverlist{
	int active[MAXSERVERS];
	struct sockaddr_in addr[MAXSERVERS];
};

// G
int session_port, frontend_len, client_len, server_len, primary_id;
SOCKET fe_socket;
struct sockaddr client;
struct sockaddr_in server;
struct sockaddr_in frontend;
struct serverlist serverlist;
struct hostent *serv_addr;

int switchservers(){
	int i = 0;
	while(i < MAXSERVERS && serverlist.active[i] == 0){
		i++;
	}
	primary_id = i;
}

int update_server_list(struct packet reply){
	char *data;
	strncpy(data,reply.data, sizeof(struct serverlist));
	serverlist = *((struct serverlist *) &data);
}

void *ping(){
	int from_len;
	struct sockaddr from;
	SOCKET session_socket;
	struct sockaddr_in rm;
	struct sockaddr_in session;
	struct packet request, reply;
	int session_len, rm_len = sizeof(struct sockaddr_in), active = 1;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	// Socket setup
	if((session_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Socket creation failure.\n");
		exit(1);
	}
	memset((void *) &session,0,sizeof(struct sockaddr_in));
	session.sin_family = AF_INET;
	session.sin_addr.s_addr = htonl(INADDR_ANY);
	session.sin_port = htons(5000);
	session_len = sizeof(session);
	if (bind(session_socket,(struct sockaddr *) &session, session_len)) {
		printf("Binding error\n");
		active = 0;
	}
	// Set timeout option
	if (setsockopt(session_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}
	rm = server;
	rm.sin_port = htons(5000);
	while(active){
		// Send ping
		request.opcode = PING;
		request.seqnum = FRONTEND; 
		sendto(session_socket, (char *) &request, PACKETSIZE, 0, (struct sockaddr *)&rm, (struct socklen_t *) &rm_len);
		// Wait for an ack & slist
		if (0 > recvfrom(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *) &from, (struct socklen_t *) &from_len)){
			// Timeout
			serverlist.active[primary_id] = 0;
			int switchservers();
			rm = serverlist.addr[primary_id];
		}
		else{
			update_server_list(reply);
		}
	}
}

void *session(){
	int i;
	SOCKET session_socket;
	struct sockaddr client;
	struct sockaddr_in session;
	struct packet request, reply;
	int from_len;
	struct sockaddr from;
	int session_len, client_len = sizeof(struct sockaddr_in), active = 1;
	struct timeval tv;
	tv.tv_sec = 100;
	tv.tv_usec = 0;
	// Socket setup
	if((session_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Socket creation failure.\n");
		exit(1);
	}
	memset((void *) &session,0,sizeof(struct sockaddr_in));
	session.sin_family = AF_INET;
	session.sin_addr.s_addr = htonl(INADDR_ANY);
	session.sin_port = htons((int) session_port);
	session_len = sizeof(session);
	if (bind(session_socket,(struct sockaddr *) &session, session_len)) {
		printf("Binding error\n");
		active = 0;
	}
	for (i = 0; i < MAXSERVERS; i++){
		serverlist.active[i] = 0;
	}
	while(active){
		// Wait for a packet from client
		recvfrom(session_socket, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &client, sizeof(struct sockaddr_in));
		// Reroute to server
		sendto(session_socket, (char *) &request, PACKETSIZE, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
		// Wait for a reply packet from server
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		if (setsockopt(session_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
			perror("Error");
		}
		recvfrom(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *) &from, sizeof(struct sockaddr_in));
		// Reroute to client
		sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, sizeof(struct sockaddr_in));
	}
}

int main(int argc,char *argv[]){
	char host[20];
	int online = TRUE;
	struct packet request, reply;
	int from_len;
	struct sockaddr from;
	pthread_t tid1, tid2;
	// Get server address
	if (argc!=2){
		printf("Escreva no formato: ./frontEnd <endereço_do_server_primario>\n\n");
		return 0;
	}
	strcpy(host,argv[1]);
	serv_addr = gethostbyname(host);
	server.sin_family = AF_INET;
	server.sin_addr = *((struct in_addr *)serv_addr->h_addr);
	server.sin_port = htons(MAIN_PORT);
	server_len = sizeof(server);
	// Socket setup
	if((fe_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Socket creation failure.\n");
		exit(1);
	}
	memset((void *) &frontend,0,sizeof(struct sockaddr_in));
	frontend.sin_family = AF_INET;
	frontend.sin_addr.s_addr = htonl(INADDR_ANY);
	frontend.sin_port = htons(MAIN_PORT);
	frontend_len = sizeof(frontend);
	if (bind(fe_socket,(struct sockaddr *) &frontend, frontend_len)) {
		printf("Binding error\n");
		exit(1);
	}
	printf("Socket initialized, waiting for requests.\n\n");
	// Setup done

	// Login request
	recvfrom(fe_socket, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &client, sizeof(struct sockaddr_in));
	// Login reroute
	sendto(fe_socket, (char *) &request, PACKETSIZE, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
	// Login reply
	recvfrom(fe_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *) &from, sizeof(struct sockaddr_in));
	session_port = reply.seqnum;
	// Login reply reroute to client
	sendto(fe_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, sizeof(struct sockaddr_in));
	// call ping
 	pthread_create(&tid1, NULL, ping, NULL);
	// call session
	pthread_create(&tid2, NULL, session, NULL);
	return 0;
>>>>>>> 2ab3a08bd6c3d5bd7c633689c43cd58ebf29261a
}
