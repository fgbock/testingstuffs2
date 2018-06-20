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
#define MAXSERVERS 5
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
#define PING 10
#define NONE 1000
#define PORTRM 5999

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
	struct sockaddr_in addr[MAXSERVERS];
	int active[MAXSERVERS];
};

// Global Variables
struct client client_list [MAXCLIENTS];
char endprimario[20];
int port;
int isPrimario = FALSE;
int is_pinging = FALSE;
double time_between_ping = 10.f;
double time_to_timeout = 5.f;
struct sockaddr_in prim_addr;
struct hostent *primario;
int socket_rm;
struct serverlist serverlist;
int server_id;
struct sockaddr_in serv_addr;
struct hostent *jbserver;
pthread_mutex_t lockcomunicacao;

//================PART 1=================================================
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
	strcpy(path, getenv("HOME"));
	strcat(path, "/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);
	printf("File path is: %s\n\n",path);
	receive_file_from(socket, path);
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
				break;
			case CLOSE:
				reply.opcode = ACK;
				sendto(session_socket, (char *) &reply, PACKETSIZE, 0, (struct sockaddr *)&client, client_len);
				client_list[c_id].session_active[s_id] = 0;
				pthread_exit(0); // Should have an 'ack' by the client allowing us to terminate, ideally!
				break;
			default:
				printf("ERROR: Invalid packet detected. Type %hi, seqnum: %hi.\n\n",request.opcode, request.seqnum);
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
//=======================================================================
//================PART 2=================================================
void eleicaodeprimario(){
	 int i;
	for (i = server_id; i < MAXSERVERS; i++){
		// send election request

	}
}

void atualizalistaservidores(){

}

void pingPrimario(){
	int n;
	struct sockaddr_in from;
	char buffer[PACKETSIZE];
	int i;
	int recebeuack = FALSE;
	struct packet message, reply;
	unsigned int length = sizeof(struct sockaddr_in);
	int ret;
	FILE *fp;
	char path[300];
	double last_time;
	double actual_time;
	int deutimeout = FALSE;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	if (setsockopt(socket_rm, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    perror("Error");
	}

	pthread_mutex_lock(&lockcomunicacao);

	message.opcode = PING;
	message.seqnum = NONE;
	strcpy(message.data,"");

	last_time=  (double) clock() / CLOCKS_PER_SEC;
	actual_time = (double) clock() / CLOCKS_PER_SEC;
	while(!recebeuack && !deutimeout){
		actual_time = (double) clock() / CLOCKS_PER_SEC;
		n = sendto(socket_rm, (char *)&message, PACKETSIZE, 0, (const struct sockaddr *) &prim_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_rm, (char *)&reply, PACKETSIZE, 0, (struct sockaddr *) &from, &length);

		if (actual_time - last_time >= time_to_timeout){ //throws a ping thread every 10 sec, if there isn't one already
			last_time = actual_time;
			deutimeout = TRUE;
			eleicaodeprimario();
		}
		if (reply.opcode == ACK){
			recebeuack = TRUE;
			if (message.seqnum == NONE){
				message.seqnum = reply.seqnum; // Pega id da lista via seq num
				server_id = (int) reply.seqnum;
			}
		}
	}

	if(deutimeout == FALSE){
		atualizalistaservidores(reply.data);
	}
}

void* thread_ping(void *vargp){
		is_pinging = TRUE;
		pingPrimario();
		is_pinging = FALSE;
    pthread_exit((void *)NULL);
}

int insert_new_server(struct sockaddr_in new_server){
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

void waitforpings(){
	int n;
	int length;
	struct sockaddr_in from;
	struct packet message, reply;
	strcpy(reply.data,(char *) &serverlist);

	while(TRUE && isPrimario){
		n = recvfrom(socket_rm, (char *)&message, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
		if (message.seqnum == NONE){
			n = insert_new_server(from);
			if (n >= 0){
				reply.seqnum = n;
				reply.opcode = ACK;
			}
			else{
				reply.opcode = NACK;
			}
		}
		n = sendto(socket_rm, (char *)&reply, PACKETSIZE, 0, (const struct sockaddr *) &from, sizeof(struct sockaddr_in));
	}
}

void* thread_rm_pings(void *vargp){
	pthread_t tid[2];
	double last_time;
	double actual_time;
	last_time=  (double) clock() / CLOCKS_PER_SEC;
	actual_time = (double) clock() / CLOCKS_PER_SEC;
	while(TRUE){
		actual_time = (double) clock() / CLOCKS_PER_SEC;
		if (isPrimario){
			waitforpings();
		}
		else{
			if ((!is_pinging)&&(actual_time - last_time >= time_between_ping)){ //throws a ping thread every 10 sec, if there isn't one already
				last_time = actual_time;
				pthread_create(&(tid[0]), NULL, thread_ping, NULL);
			}
		}
	}
	pthread_exit((void *)NULL);
}


int main(int argc,char *argv[]){
	pthread_t tid;
	//char host[20];
	SOCKET main_socket;
	struct sockaddr client;
	struct sockaddr_in server;
	struct packet login_request, login_reply;
	int i, j, session_port, server_len, client_len = sizeof(struct sockaddr_in), online = 1;

	if (argc!=2){
		printf("Escreva no formato: ./dropboxServer <endereço_do_server_primario>\n");
	}
	else{ //wrote correcly the arguments

		//===================================PART 2============================
		strcpy(endprimario,argv[1]);
		if(strcmp(endprimario,"127.0.0.1")==0){
			isPrimario = TRUE;
		}
		else{
			isPrimario = FALSE;
		}

		if ((socket_rm = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
			printf("ERROR opening socket");
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(PORTRM);
		serv_addr.sin_addr = *((struct in_addr *)jbserver->h_addr);
		bzero(&(serv_addr.sin_zero), 8);

		printf("GOT HERE 0 \n\n");
		serverlist.active[0] = 1;
		serverlist.addr[0] = serv_addr;
		for(i = 1; i < MAXSERVERS; i++){
			serverlist.active[i] = 0;
		}
		printf("GOT HERE 2\n\n");
		pthread_create(&tid, NULL, thread_rm_pings, NULL);
		printf("GOT HERE 3\n\n");
		//==================================================================

		for (i = 0; i < MAXCLIENTS; i++){
			for(j = 0; j < MAXSESSIONS; j++){
				client_list[i].socket_set[j] = 0;
			}
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
	}


	return 0;
}

#endif
