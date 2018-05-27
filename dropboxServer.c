#ifndef DROPBOXSERVER_C
#define DROPBOXSERVER_C

#define SOCKET int
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include "dropboxUtils.h"
#include <dirent.h>

#define MAIN_PORT 6000

// Structures
struct file_info {
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
};

struct client {
	SOCKET socket;
	char userid [MAXNAME];
	struct file_info info [MAXFILES];
	//int logged_in;
};

struct session {
	//int client_id;
	int can_receive;
	int client_address_len;
	struct sockaddr client_address;
	char session_buffer[1250];
	int active;
};

// Global Variables
//struct client client_list[10];
//struct session session_list[20];
//int client_active[10];
//int session_active[20];
int meme;
struct session session_info_1;
struct session session_info_2;
struct client client_info;

// Specification subroutines
void sync_server(int socket,  char*userID){
	int n_files = receive_int_from(socket); //número de arquivos a serem atualizados

	for(int i=0; i<n_files; i++){
		char *file = receive_string_from(socket); //cliente manda nome do arquivo a ser alterado/criado

		char *path = malloc(sizeof(char)*(strlen(userID)+17+strlen(file)));

		strcpy(path, "~/dropboxserver/");
		strcat(path, userID);
		strcat(path, "/");
		strcat(path, file); //arquivo será salvo no server


		receive_file_from(socket, path,session_info_1.client_address); //cliente manda o arquivo

		free(file);
		free(path);
	}
	return;
}

void send_file(char *file, int socket, char *userID, int session_id){
	//forma o path do arquivo no servidor com base no userid e nome do arquivo
	char *path = malloc(sizeof(char)*(strlen(userID)+17+strlen(file)));

	strcpy(path, "~/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);

	//send_string_to(socket, path);//este path pode ser como o clente ira salvar o file
	if (session_id == 1){
		send_file_to(socket, path, session_info_1.client_address);//evia o arqivo para o cliente. O cliente deverá escolher o nome do arquivo gravado com o receive_file
	}
	else if (session_id == 2){
		send_file_to(socket, path, session_info_2.client_address);//evia o arqivo para o cliente. O cliente deverá escolher o nome do arquivo gravado com o receive_file
	}
	free(path);
}

void receive_file(char *file, int socket, char*userID){
	int i;
	//forma o path do arquivo no servidor com base no userid e nome do arquivo
	char *path = malloc(sizeof(char)*(strlen(userID)+17+strlen(file)));

	strcpy(path, "~/dropboxserver/");
	strcat(path, userID);
	strcat(path, "/");
	strcat(path, file);

	//send_string_to(socket, path);//este path pode ser como o clente ira salvar o file
	printf("... %s\n",path);
	i = receive_file_from(socket, path,session_info_1.client_address);//recebe o arquivo do cliente no path montado. o arquivo pode estar em qualquer lugar no cliente
	printf("\n\nGod\n");
	free(path);
	printf("\nDevil\n");
}

// Auxiliary Functions

int delete_file(char *file, int socket, char*userID){
	if(remove(file) == 0){
		return 1;
	}
	return 0;
}

void list_files(SOCKET socket, struct sockaddr thing){
  DIR *dp;
  struct dirent *ep;
	char saida[110];
	strcpy(saida,"ACKlist_f0000\0");

  dp = opendir ("./");
  if (dp != NULL)
    {
      while (ep = readdir (dp)){
        strcat(saida,ep->d_name);
    }
    sendto(socket,saida,sizeof(saida),0,(struct sockaddr *)&thing, sizeof(thing));
      (void) closedir (dp);
    }
  else
    perror ("Couldn't open the directory");
}

int min(int right, int left){
	if(right > left){
		return left;
	}
	else{
		return right;
	}
}

/*
int get_session_spot(){
	int i;
	for (i = 0; i < 20; i++){
		if(session_active[i] == 0){
			return i;
		}
	}
	return -1;
}
*/

/*
int get_sequence_num(int s_id){
	char aux_str[5];
	int i, value;
	for(i = 0; i < 4; i++){
		aux_str[i] = session_list[s_id].session_buffer[6+i];
	}
	aux_str[4] = '\0';
	value = *(int *) aux_str;
	printf("Val is: %d", value);
	return value;
}
*/

/*
int socket_cmp(struct session *left, struct session *right){
    socklen_t min_address_len = min(left->client_address_len, right->client_address_len);
    // If head matches, longer is greater.
    int default_rv = right->client_address_len - left->client_address_len;
    int rv = memcmp(left, right, min_address_len);
    return rv;
}
*/

int redirect_package(char packet_buffer[1250], struct sockaddr client, int client_len){
	int i;
	/*
	struct session new_session;
	new_session.client_address = client;
	new_session.client_address_len = client_len;
	*/
	//printf("Client sa_data : %s\n",client.sa_data);

	//if (socket_cmp(&session_info_1,&new_session) == 0 && session_info_1.can_receive){
	if (strcmp(client.sa_data,session_info_1.client_address.sa_data) == 0 && session_info_1.can_receive){
		session_info_1.can_receive = 0;
		strcpy(session_info_1.session_buffer,packet_buffer);
		printf("session buffer: %s packet buffer: %s",session_info_1.session_buffer,packet_buffer);
		return 0;
	}
	//else if (socket_cmp(&session_info_1,&new_session) == 0 && session_info_1.can_receive){
	else if (strcmp(client.sa_data,session_info_2.client_address.sa_data) == 0 && session_info_2.can_receive){
		session_info_1.can_receive = 0;
		strcpy(session_info_2.session_buffer,packet_buffer);
		return 0;
	}
	//printf("teste redirect\n\n");
	return 0;
	/*
	for(i = 0; i < 20; i++){
		if (socket_cmp(&(session_list[i]),new_session)== 0 && session_list[i].can_receive){
			session_list[i].can_receive = 0;
			strcpy(session_list[i].session_buffer,packet_buffer);
			return 0;
		}
	}
	*/
 	return 1;
}

void *session_manager(void *args){
	char op_code[7];
	int seq_num, online = 1, c_id, aux;
	int *s_id = (int *) args;
	char * argument;
	char packet_buffer[100];
	int sockfd, n;
	unsigned int length;
	struct sockaddr_in serv_addr, from;
	struct hostent *server;
	struct session* current_session;
	int i;
	if ((*s_id) == 1){
		current_session = &session_info_1;
	}
	else{
		current_session = &session_info_2;
	}
	struct sockaddr client_addr = current_session->client_address;
	length = current_session->client_address_len;

	printf("coisa por session manager \n");
	for(i =0;i<14;i++){
		printf("%d",session_info_1.client_address.sa_data[i]);
	}
	printf("\n\n");
	//struct sockaddr* client_addr = &(session_list[s_id].client_address);
	SOCKET socket = client_info.socket;
  	while(online){
		if((*current_session).can_receive == 0){
			//printf("Time to handle a request...\n");
      		//n = recvfrom(sockfd, packet_buffer, strlen(packet_buffer), 0, (struct sockaddr *) &from, &length);
			if ((*s_id) == 1){
				current_session = &session_info_1;
				strcpy(packet_buffer, session_info_1.session_buffer);
				printf("Bado: %s\n",packet_buffer);
				printf("Buceta de touro: %s\n",session_info_1.session_buffer);
			}
			else{
				current_session = &session_info_2;
				strcpy(packet_buffer, session_info_2.session_buffer);
			}
		//printf("\npacket na session: %s \n", packet_buffer);
	strncpy(op_code,packet_buffer,6);
			op_code[6] = '\0';
			if(op_code[0] != '\0'){
				printf("Opcode is %s\n\n",op_code);
			}
			if(n>0){
				printf("entrou nessa coisa %s\n",op_code);
			}
			if (!strcmp(op_code,"downlo")){
        		argument = getArgument(packet_buffer);
        		send_file(argument,socket,client_info.userid,*s_id);
			}
			else if (!strcmp(op_code,"upload")){
        		argument = getArgument(packet_buffer);
						sendto(socket,"ACKupload0000",sizeof("ACKcloses0000"),0,(struct sockaddr *)&session_info_1.client_address, sizeof(session_info_1.client_address));
        		receive_file(argument,socket,client_info.userid);
			}
      		else if (!strcmp(op_code,"delete")){
        		argument = getArgument(packet_buffer);
        		if (delete_file(argument,socket,client_info.userid)){
        			sendto(socket,"ACKdelete0000",sizeof("ACKdelete0000"),0,(struct sockaddr *)&client_addr, sizeof(client_addr));
        		}
			}
      		else if (!strcmp(op_code,"list_f")){
       			argument = getArgument(packet_buffer);
        		list_files(socket,client_addr);
			}
					else if (strcmp(op_code,"closes")){
						//printf("entrou no close\n");
						(*current_session).active = 0;
						sendto(socket,"ACKcloses0000",sizeof("ACKcloses0000"),0,(struct sockaddr *)&session_info_1.client_address, sizeof(session_info_1.client_address));
						/*
						session_active[s_id] = 0;
						c_id = session_list[s_id].client_id;
						client_list[c_id].logged_in = client_list[c_id].logged_in + 1;
						sendto(*socket,"ACKcloses0000",sizeof("ACKcloses0000"),0,(struct sockaddr *)&client_addr, sizeof(client_addr));
						*/
			}
			strcpy(op_code,"\0");
		}
	}
}

int login(char packet_buffer[1250], struct sockaddr client, int client_len, SOCKET s_socket){
 	char aux_username[20];
	//struct client new_client;
	//struct session new_session;
	pthread_t tid;
	int i, not_done = 1, aux_index, s_id;
	// Verify client list

	strncpy(aux_username,&(packet_buffer[10]),20);
	/*
	aux_index = get_session_spot();
	if (aux_index == -1){
		return 0;
	}
	session_active[aux_index] = 1;
	*/
	create_server_userdir(aux_username);
	if(meme){
		strcpy(client_info.userid,aux_username);
		meme = 0;
	}
	if(session_info_1.active == 0){
		session_info_1.client_address = client;
		session_info_1.client_address_len = client_len;
		session_info_1.can_receive = 1;
		session_info_1.active = 1;
		s_id = 1;
		//pthread_create(&tid, NULL, session_manager, &s_id);
		return 1;
	}
	else if (session_info_2.active == 0){
		session_info_2.client_address = client;
		session_info_2.client_address_len = client_len;
		session_info_2.can_receive = 1;
		session_info_2.active = 1;
		//pthread_create(&tid, NULL, session_manager, &s_id);
		return 1;
	}
	else{
		return 0;
	}

	/*
	for(i = 0; i < 10; i++){
		if(strcmp(client_list[i].userid,aux_username) == 0){
			if(client_list[i].logged_in == 0){
				return 0;
			}
			else{
				client_list[i].logged_in = 0; // Update logged_in
				new_session.client_id = i;
				new_session.client_address = client;
				new_session.client_address_len = client_len;
				new_session.can_receive = 1;
				new_session.socket = s_socket;
				session_list[aux_index] = new_session;
				printf("New session created, added device to active client:\n");
				pthread_create(&tid, NULL, session_manager, &aux_index);
				return 1;
			}
		}
	}
	for(i = 0; i < 10; i++){
		if(client_active[i] == 0){
			strcpy(new_client.userid, aux_username); // Set userid
			new_client.logged_in = 1; // Set logged_in
			client_list[i] = new_client; // Place new client into the client list
			new_session.client_id = i; // Inform the session of the client's index in the list
			new_session.client_address = client;
			new_session.client_address_len = client_len;
			new_session.can_receive = 1;
			new_session.socket = s_socket;
			session_list[aux_index] = new_session;
			printf("New session created:\n");
			pthread_create(&tid, NULL, session_manager, &aux_index);
			return 1;
		}
	}
	*/
}

// Server's main thread
int main(int argc,char *argv[]){
	struct sockaddr_in server;
	struct sockaddr client;
	SOCKET s_socket;
	int server_len, received, i, online = 1, client_len = sizeof(struct sockaddr_in), login_value, seq_num;
	char packet_buffer[1250];
	char reply_buffer[1250];
	char ack_buffer[20];
	char op_code[7];
	char * argument;
	char userid[20];
	meme = 1;
	// Initializing client list (temporary measure - until we get a user list file)
	/*
	for(i = 0; i < 10; i++){
		struct client new_client;
		client_list[i] = new_client;
		client_list[i].logged_in = 2;
		strcpy(client_list[i].userid," ");
		client_active[i] = 0;
	}
	for (i = 0; i < 20; i++){
		session_active[i] = 0;
	}
	*/

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

	// WOW:
	client_info.socket = s_socket;
	/*
		Main loop - receives packages and either
			1) redirects them to a session thread, where:
				- it updates the folder (receive_file or sync are called)
				- it sends a file (sync_client or send_file are called)
			2) uses their info to create a new session thread
	*/
	create_server_root();
	while(online){
		received = recvfrom(s_socket,packet_buffer,sizeof(packet_buffer),0,(struct sockaddr *) &client,(socklen_t *)&client_len);
		if (!received){
			printf("ERROR: Package reception error.\n\n");
		}
		//printf("\n%s\n", packet_buffer);
		strncpy(op_code,packet_buffer,6);
		op_code[6] = '\0';
		strncpy(userid,&(packet_buffer[10]),20);
		if (strcmp(op_code,"logins") == 0){
			if (login(packet_buffer, client, client_len, s_socket)){
				strcpy(ack_buffer,"ACKlogins0000");
				/*for(i = 0; i < 4; i++){
					ack_buffer[6+i] = packet_buffer[6+i];
				}*/
				//ack_buffer[10] = '\0';
				sendto(s_socket,ack_buffer,strlen(ack_buffer),0,(struct sockaddr *)&client, client_len);
				printf("Login succesful...\n");
			}
			else{
				strcpy(ack_buffer,"NOTACK");
				for(i = 0; i < 4; i++){
					ack_buffer[6+i] = packet_buffer[6+i];
				}
				//ack_buffer[10] = '\0';
				sendto(s_socket,ack_buffer,sizeof(ack_buffer),0,(struct sockaddr *)&client, client_len);
				printf("ERROR: Login unsuccesful...\n");
			}
		}
		if (strcmp(op_code,"downlo") == 0){
        	argument = getArgument(packet_buffer);
        	send_file(argument,s_socket,userid,1);
		}
		else if (strcmp(op_code,"upload") == 0){
        	argument = getArgument(packet_buffer);
			sendto(s_socket,"ACKupload0000",sizeof("ACKupload0000"),0,(struct sockaddr *)&client, sizeof(session_info_1.client_address));
        	receive_file(argument,s_socket,userid);
		}
      	else if (strcmp(op_code,"delete") == 0){
        	argument = getArgument(packet_buffer);
        	if (delete_file(argument,s_socket,userid)){
        		sendto(s_socket,"ACKdelete0000",sizeof("ACKdelete0000"),0,(struct sockaddr *)&client, sizeof(client));
        	}
		}
      	else if (strcmp(op_code,"list_f") == 0){
       		argument = getArgument(packet_buffer);
        	list_files(s_socket,client);
		}
		else if (strcmp(op_code,"closes") == 0){
			printf("Error\n\n");
			session_info_1.active = 0;
			sendto(s_socket,"ACKcloses0000",sizeof("ACKcloses0000"),0,(struct sockaddr *)&client, sizeof(session_info_1.client_address));
		}
		printf("Success\n");
		strcpy(op_code,"\0");
		memset(packet_buffer,0,1250);
		memset(op_code,0,7);
	}
	return 0;
}


#endif
