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

// Global Variables
struct client client_list [MAXCLIENTS];

// Global Variables Part II
char host[20];
struct serverlist serverlist;
int primary_id;
int server_id;
int is_primary;
SOCKET rep_socket;

// Subroutines
void replication(struct packet message){
	int i;
	struct sockaddr_in destination;
	for(i = 0; i < MAXSERVERS; i++){
		if(i != server_id){
			destination = serverlist.addr[i];
			sendto(rep_socket, (char *) &message, PACKETSIZE, 0, (struct sockaddr *)&destination, sizeof(struct sockaddr_in));
		}
	}
}

char * devolvePathHomeServer(char *userID){
	char * pathsyncdir;
	pathsyncdir = (char*) malloc(sizeof(char)*100);
	strcpy(pathsyncdir,"~/");
	removeBlank(pathsyncdir);
	strcat(pathsyncdir,"dropboxserver/");
	strcat(pathsyncdir,userID);
	strcat(pathsyncdir,"/");


	return pathsyncdir;
}

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
	char path[300];
	strcpy(path, getenv("HOME"));
	strcat(path, "/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);
	printf("File path is: %s\n\n",path);
	send_file_to(socket, path, client_addr);
}

void receive_file(char *file, int socket, char*userID){
	char path[300];
	int i;
	struct sockaddr *dest;
	strcpy(path, getenv("HOME"));
	strcat(path, "/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);
	printf("File path is: %s\n\n",path);
	receive_file_from(socket, path);
	for(i = 0; i < MAXSERVERS; i++){
		dest = (struct sockaddr *) &(serverlist.addr[i]);
		send_file_to(socket, path, *dest);
	}
}

int delete_file(char *file, int socket, char*userID){
	char path[300];
	strcpy(path, getenv("HOME"));
	strcat(path, "/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);
	if(remove(path) == 0){
		return 1;
	}
	return 0;
}

void list_files(SOCKET socket, struct sockaddr client, char *userID){
	char * path;
	int length, i = 0;
	int fd;
	int wd;
	struct dirent *ep;
	char files_list[PACKETSIZE - 4];
	struct packet reply;
	reply.opcode = ACK;

	path = devolvePathHomeServer(userID);
	DIR* dir = opendir(path);
	struct dirent * file;
	strcpy(files_list,"Conteúdo do diretório remoto:\n");
	while((file = readdir(dir)) != NULL){
		if(file->d_type==DT_REG){
			strcat(files_list," - ");

			strcat(files_list,file->d_name);
			strcat(files_list,"\n");
		}
	}

	strncpy(reply.data, files_list, PACKETSIZE - 4);
	sendto(socket, (char *) &reply, PACKETSIZE,0,(struct sockaddr *)&client, sizeof(client));
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
	//printf("\nClient Id is %d and  Session Id is %d\n\n", c_id, s_id);
	session_port = (int) client_list[c_id].session_port[s_id];

	if (client_list[c_id].socket_set[s_id] == 0){
		// Set up a new socket
		printf("Setting up a new socket for this session!\n\n");
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
		else{
			printf("Client %d, Session %d Socket initialized, waiting for requests.\n\n", c_id, s_id);
		}
		client_list[c_id].socket_set[s_id] = 1;
		client_list[c_id].socket[s_id] = session_socket;
	}
	else{
		session_socket = client_list[c_id].socket[s_id];
	}
	printf("Client %d, Session %d Port is %hi\n\n", c_id, s_id, session_port);
		// Setup done

	while(active){
		if (!recvfrom(session_socket, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &client, (socklen_t *) &client_len)){
			printf("ERROR: Package reception error.\n\n");
		}
		printf("Client %d, Session %d Opcode is: %hi\n\n", c_id, s_id, request.opcode);
		switch(request.opcode){
			case UPLOAD:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				strncpy(filename, request.data, MAXNAME);
				receive_file(filename, session_socket, client_list[c_id].user_id);
				replication(request);
				break;
			case DOWNLOAD:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				strncpy(filename, request.data, MAXNAME);
				send_file(filename, session_socket, client_list[c_id].user_id, client);
				break;
			case LIST:
				reply.opcode = ACK;
				//sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				list_files(session_socket, client, client_list[c_id].user_id);
				break;
			case DELETE:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				strncpy(filename, request.data, MAXNAME);
				delete_file(filename, session_socket, client_list[c_id].user_id);
				replication(request);
				break;
			case CLOSE:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				client_list[c_id].session_active[s_id] = 0;
				replication(request);
				pthread_exit(0); // Should have an 'ack' by the client allowing us to terminate, ideally!
				break;
			default:
				printf("ERROR: Invalid packet detected. Type %hi, seqnum: %hi.\n\n",request.opcode, request.seqnum);
		}
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

// ========================================================================== //


void elect_primary(){
	int i = 0;
	while(i < MAXSERVERS && serverlist.active[i] == 0){
		i++;
	}
	if (i < MAXSERVERS){
		primary_id = i;
	}
	if (primary_id == server_id || i >= MAXSERVERS){
		is_primary = TRUE;
		primary_id = server_id;
	}
}

int update_server_list(struct packet reply){
	char *data;
	int i;
	memcpy((void *) &serverlist,reply.data,sizeof(data));
	for(i = 0; i < MAXSERVERS; i++){
		printf("Server #%d is %d\n\n",i, serverlist.active[i]);
	}
}

int insert_in_server_list(struct sockaddr_in new_server){
	int i = 0;
	while(i < MAXSERVERS && serverlist.active[i] != 0){
		i++;
	}
	if (i < MAXSERVERS){
		serverlist.active[i] = 1;
		serverlist.addr[i] = new_server;
		return i;
	}
	return -1;
}

void *replica_manager(){
	SOCKET rm_socket;
	struct sockaddr_in primary_rm, this_rm, from;
	struct packet ping, ping_reply;
	int i, j, rm_port, this_len, from_len, primary_len = sizeof(struct sockaddr_in), online = 1;
	struct hostent *primary_host;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	ping.opcode = PING;
	ping.seqnum = NONE;
	// Socket setup
	server_id = -1;
	rm_port = 5000; // Set port!
	if((rm_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		exit(1);
	}
	memset((void *) &this_rm,0,sizeof(struct sockaddr_in));
	this_rm.sin_family = AF_INET;
	this_rm.sin_addr.s_addr = htonl(INADDR_ANY);
	this_rm.sin_port = htons(rm_port);
	this_len = sizeof(this_rm);
	if (bind(rm_socket,(struct sockaddr *) &this_rm, this_len)) {
		exit(1);
	}
	if (strcmp(host,"127.0.0.1") == 0){
		is_primary = TRUE;
		ping.seqnum = 0;
		server_id = insert_in_server_list(this_rm);
	}
	else{
		printf("Host is %s\n\n",host);
		primary_host = gethostbyname(host);
		primary_rm.sin_family = AF_INET;
		primary_rm.sin_port = htons(5000);
		primary_rm.sin_addr = *((struct in_addr *)primary_host->h_addr);
		bzero(&(primary_rm.sin_zero), 8);
	}

	while(online){
		printf("Primary is %d\n\n", primary_id);
		// Check if you're the rm_primary
		if (is_primary){
			tv.tv_sec = 25;
			tv.tv_usec = 0;
			if (setsockopt(rm_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
				perror("Error");
			}
			recvfrom(rm_socket, (char *) &ping, PACKETSIZE, 0, (struct sockaddr *) &from, (socklen_t *) &from_len);
			if (ping.opcode == PING){
				if(ping.seqnum == NONE){
					ping_reply.seqnum = insert_in_server_list(from);
				}
				ping_reply.opcode = ACK;
				strncpy(ping_reply.data, (char *) &serverlist, sizeof(struct serverlist));
				sendto(rm_socket, (char *) &ping_reply, PACKETSIZE, 0, (struct sockaddr *)&from, primary_len);
			}
		}
		else{
			// Send ping
			sendto(rm_socket, (char *) &ping, PACKETSIZE, 0, (struct sockaddr *)&primary_rm, primary_len);
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			if (setsockopt(rm_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
				perror("Error");
			}
			if (0 > recvfrom(rm_socket, (char *) &ping_reply, PACKETSIZE, 0, (struct sockaddr *) &from, (socklen_t *) &from_len)){
				//Timeout!
				serverlist.active[primary_id] = 0;
				elect_primary();
				primary_rm = serverlist.addr[primary_id];
			}
			else if(ping.seqnum == NONE){
				ping.seqnum = ping_reply.seqnum;
				server_id = (int) ping.seqnum;
			}
			update_server_list(ping_reply);
		}
	}
	return 0;
}

void *setrep(){
	struct sockaddr_in rep;
	int rep_len;
	if((rep_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Socket creation failure.\n");
		exit(1);
	}
	memset((void *) &rep,0,sizeof(struct sockaddr_in));
	rep.sin_family = AF_INET;
	rep.sin_addr.s_addr = htonl(INADDR_ANY);
	rep.sin_port = htons(4000);
	rep_len = sizeof(rep);
	if (bind(rep_socket,(struct sockaddr *) &rep, rep_len)) {
		printf("Rep Binding error\n");
		exit(1);
	}
	printf("Rep socket initialized.\n\n");
	pthread_exit(0);
}

int main(int argc,char *argv[]){
	//char host[20];
	SOCKET main_socket;
	struct sockaddr client;
	struct sockaddr_in server;
	struct packet login_request, login_reply;
	int i, j, session_port, server_len, client_len = sizeof(struct sockaddr_in), online = 1;
	pthread_t tid1, tid2;

	if (argc!=2){
		printf("Escreva no formato: ./dropboxServer <endereço_do_server_primario>\n\n");
		return 0;
	}
	strcpy(host,argv[1]);
	is_primary = FALSE;
	for (i = 0; i < MAXCLIENTS; i++){
		for(j = 0; j < MAXSESSIONS; j++){
			client_list[i].socket_set[j] = 0;
		}
	}
	for (i = 0; i < MAXSERVERS; i++){
		serverlist.active[i] = 0;
	}

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

	pthread_create(&tid1, NULL, setrep, NULL);
	pthread_create(&tid2, NULL, replica_manager, NULL);

	while(online){
		if (!recvfrom(main_socket, (char *) &login_request, PACKETSIZE, 0, (struct sockaddr *) &client, (socklen_t *) &client_len)){
			printf("ERROR: Package reception error.\n\n");
		}
		else{
			session_port = login(login_request);
			//printf("\nopcode is %hi\n\n",login_request.opcode);
			//printf("\ndata is %s\n\n",login_request.data);
			if (session_port > 0){
				login_reply.opcode = ACK;
				login_reply.seqnum = (short) session_port;
				sendto(main_socket, (char *) &login_reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				replication(login_request);
				printf("Login succesful...\n\n");
			}
			else{
				login_reply.opcode = NACK;
				sendto(main_socket, (char *) &login_reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				printf("ERROR: Login unsuccesful...\n\n");
			}
			//
		}
		login_request.opcode = 0;
	}

	return 0;
}

#endif
