#ifndef DROPBOXCLIENT_C
#define DROPBOXCLIENT_C

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "dropboxUtils.h"
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <netdb.h>
#include <pwd.h>

#define BUFFERSIZE 1250
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

struct packet {
	short int opcode;
	short int seqnum;
	char data [PACKETSIZE - 4];
};
int is_syncing = FALSE;
int mustexit = FALSE;
char userID[20];
char host[20];
int port;
int socket_local;
struct sockaddr_in serv_addr;
struct hostent *server;
double time_between_sync = 10000.f;

//=======================================================
void pickFileNameFromPath(char *path,char *filename){
	char aux[100]; char aux2[100];
	int i=0;int j=0;
	int lastdash=0;
	strcpy(aux,path);

	while(aux[i]!='\0'){
		if(aux[i]=='/')
			lastdash = i;
		i++;
	}
	strcpy(filename,&aux[lastdash+1]);

}

int login_server(char *host,int port){
	int n;
	struct sockaddr_in from;
	char buffer[BUFFERSIZE];
	int i;
	int recebeuack = FALSE;
	struct packet message, reply;
	unsigned int length = sizeof(struct sockaddr_in);

	create_home_dir(userID);

	for (i=0;i<BUFFERSIZE;i++)
		buffer[i]='\0';

	message.opcode = LOGIN;
	message.seqnum = LOGIN;
	strcpy(message.data,userID);

	while(!recebeuack){
		n = sendto(socket_local, (char *)&message, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, (char *)&reply, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
		if (reply.opcode == ACK){
			recebeuack = TRUE;
		}
	}
	int newport = reply.seqnum;

	if ((socket_local = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(newport);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);


	return 1;
}

void get_file(char *filename, char * finalpath){
	int n;
	struct sockaddr_in from;
	char buffer[BUFFERSIZE];
	int i;
	int recebeuack = FALSE;
	struct packet message, reply;
	unsigned int length = sizeof(struct sockaddr_in);
	char filepath[300];
	const char *homedir;
	char path[300];

	if ((homedir = getenv("HOME")) == NULL) {
			homedir = getpwuid(getuid())->pw_dir;
	}

	message.opcode = DOWNLOAD;
	message.seqnum = 0;
	strcpy(message.data,filename);

	while(!recebeuack){
		n = sendto(socket_local, (char *)&message, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, (char *)&reply, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
		if (reply.opcode == ACK){
			recebeuack = TRUE;
		}
	}

	strcpy(filepath,"~/");
	removeBlank(filepath);
	strcat(filepath,&finalpath[2]);
	strcat(filepath,filename);

	printf("Recebemos ack. Vamos receber o arquivo agora. Nome do arquivo era: %s\ne o path era: %s\n",filename,filepath);
	recebeuack = receive_file_from(socket_local, filepath);

}

void send_file(char *file){
	int n;
	struct sockaddr_in from;
	char buffer[BUFFERSIZE];
	int i;
	int recebeuack = FALSE;
	struct packet message, reply;
	unsigned int length = sizeof(struct sockaddr_in);
	char filepath[300];
	char pathsyncdir[100];

	struct sockaddr* destiny;
	char filename[100];

	pickFileNameFromPath(file,filename);

	message.opcode = UPLOAD;
	message.seqnum = 0;
	strcpy(message.data,filename);

	while(!recebeuack){
		n = sendto(socket_local, (char *)&message, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, (char *)&reply, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
		if (reply.opcode == ACK){
			recebeuack = TRUE;
		}
	}
	strcpy(filepath,file);
	removeBlank(filepath);
	printf("Recebemos ack. Vamos enviar o arquivo agora. Nome do arquivo era: %s\ne o path era: %s\n",filename,filepath);
	send_file_to(socket_local, filepath, *((struct sockaddr*) &serv_addr));

	strcpy(pathsyncdir,"~/");
	strcat(pathsyncdir,"sync_dir_");
	strcat(pathsyncdir,userID);
	strcat(pathsyncdir,"/");
	get_file(filename,pathsyncdir);

}

void delete_file(char *filename){
	int n;
	struct sockaddr_in from;
	char buffer[BUFFERSIZE];
	int i;
	int recebeuack = FALSE;
	struct packet message, reply;
	unsigned int length = sizeof(struct sockaddr_in);
	int ret;
	FILE *fp;
	char path[300];

	strcpy(path,"~/");
	removeBlank(path);
	strcat(path,"sync_dir_");
	strcat(path,userID);
	strcat(path,"/");
	strcat(path,filename);

	fp = fopen (path,"r");
  if (fp == NULL) {
      printf ("Arquivo não existe\n");
  }
	else{
		message.opcode = DELETE;
		message.seqnum = 0;
		strcpy(message.data,filename);

		while(!recebeuack){
			n = sendto(socket_local, (char *)&message, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			n = recvfrom(socket_local, (char *)&reply, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
			if (reply.opcode == ACK){
				recebeuack = TRUE;
			}
		}
	}
	ret = remove(path);

	if(ret == 0) {
		 printf("Arquivo foi deletado com sucesso!\n");
	} else {
		 printf("Algo deu errado...\n");
	}

}

void sync_client(){
	char path[2256];
	char sendpath[2256];
	const char *homedir;
	if ((homedir = getenv("HOME")) == NULL) {
			homedir = getpwuid(getuid())->pw_dir;
	}
	strcpy(path,homedir);
	strcat(path, "/sync_dir_");
	strcat(path, userID);


	//printf("path >> %s\n",path);
	int length, i = 0;
	int fd;
	int wd;
	char buffer[BUF_LEN];

	DIR* dir = opendir(path);
	strcat(path, "/");

	struct dirent * file;

	while((file = readdir(dir)) != NULL){
		if(file->d_type==DT_REG){
			//printf("\nread: %s", file->d_name);

			strcpy(sendpath, path);
			strcat(sendpath, file->d_name);
			strcat(sendpath, "\n");
			//printf("\nSendpAth: %s\n", sendpath);
			send_file(sendpath);

		}
	}

	/*argument = getArgument(command);
	send_file(argument);*/



	/*fd = inotify_init();
	wd = inotify_add_watch(fd,path,IN_MODIFY | IN_CREATE | IN_DELETE );
	length = read( fd, buffer, BUF_LEN );
	printf("\nLength: %d\n", length);


	if ( length < 0 ) {
		perror( "read" );
	}



	while ( i < length ) {
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
		if ( event->len ) {
			if ( event->mask & IN_CREATE ) {
				if ( event->mask & IN_ISDIR ) {
					printf( "The directory %s was created.\n", event->name );
					strcat(path,event->name);
					send_file(path);
				}
				else {
					printf( "The file %s was created.\n", event->name );
				}
			}
			else if ( event->mask & IN_DELETE ) {
				if ( event->mask & IN_ISDIR ) {
					printf( "The directory %s was deleted.\n", event->name );
				}
				else {
					printf( "The file %s was deleted.\n", event->name );
					strcat(path,event->name);
					//delete_file(path);
				}
			}
			else if ( event->mask & IN_MODIFY ) {
				if ( event->mask & IN_ISDIR ) {
					printf( "The directory %s was modified.\n", event->name );
				}
				else {
					printf( "The file %s was modified.\n", event->name );
					strcat(path, "/");
					strcat(path,event->name);
					send_file(path);
				}
			}
		}
		i += EVENT_SIZE + event->len;
	}

	( void ) inotify_rm_watch( fd, wd );
	( void ) close( fd );*/


}

void close_session(){
	int n;
	struct sockaddr_in from;
	char buffer[BUFFERSIZE];
	int i;
	int recebeuack = FALSE;
	struct packet message, reply;
	unsigned int length = sizeof(struct sockaddr_in);

	create_home_dir(userID);

	for (i=0;i<BUFFERSIZE;i++)
		buffer[i]='\0';

	message.opcode = CLOSE;
	message.seqnum = 0;
	strcpy(message.data,userID);

	while(!recebeuack){
		n = sendto(socket_local, (char *)&message, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, (char *)&reply, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
		if (reply.opcode == ACK){
			recebeuack = TRUE;
		}
	}
}
//=======================================================
void list_server(){
	int n;
	struct sockaddr_in from;
	char buffer[BUFFERSIZE];
	int i;
	int recebeuack = FALSE;
	struct packet message, reply;
	unsigned int length = sizeof(struct sockaddr_in);

	create_home_dir(userID);

	for (i=0;i<BUFFERSIZE;i++)
		buffer[i]='\0';

	message.opcode = LIST;
	message.seqnum = 0;
	strcpy(message.data,userID);

	while(!recebeuack){
		n = sendto(socket_local, (char *)&message, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, (char *)&reply, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
		if (reply.opcode == ACK){
			recebeuack = TRUE;
		}
	}

	printf("Listagem do server: %s\n",reply.data);
}

void list_client(){

  DIR *dp;
  struct dirent *ep;
	char saida[100];
	char path[256];

	strcpy(path,"~/");
	removeBlank(path);
	strcat(path,"sync_dir_");
	strcat(path,userID);
	strcat(path,"/");

  dp = opendir (path);
  if (dp != NULL)
    {
      while (ep = readdir (dp))
        strcpy(saida,ep->d_name);
				printf("%s\n",saida);
      (void) closedir (dp);
    }
  else
    perror ("Couldn't open the directory");


}

void treat_command(char command[100]){
	int result= 0;
	char * argument;

	if (!strncmp("exit",command,4)){
		close_session();
		mustexit = TRUE;
	}
	else if (!strncmp("upload",command,6)){
		argument = getArgument(command);
		send_file(argument);

		result = 1;
	}
	else if (!strncmp("download",command,8)){
		argument = getArgument(command);
		get_file(argument,getSecondArgument(command));

		result = 2;
	}
	else if (!strncmp("list_server",command,11)){
		list_server();
		result = 3;
	}
	else if (!strncmp("list_client",command,11)){
		list_client();
		result =4;
	}
	else if (!strncmp("get_sync_dir",command,12)){
		result = create_home_dir(userID);
		if(!result)
			result = 5;
	}
	else if (!strncmp("delete",command,6)){
		argument = getArgument(command);
		delete_file(argument);
		result =6;
	}


	if (!result){;
	}
	else{
		printf("Operação %d efetuada com sucesso!\n",result);
	}


	//printf("Seu comando foi: %s \n",command);
}

void *thread_sync(void *vargp){
		int must_sync = FALSE;
		is_syncing = TRUE;

		must_sync = TRUE;

		//printf("Sincronizando a pasta local...\n");
		if (must_sync){
			sync_client();
		}
		//printf("Acabou de sincronizar...\n");


		is_syncing = FALSE;
    pthread_exit((void *)NULL);
}

void *thread_interface(void *vargp){
		char command[100];
		printf("Escreva uma ação para o sistema:\n");

		while(!mustexit){
			printf(">>");
			fgets(command,100,stdin);
			treat_command(command);
		}

    pthread_exit((void *)NULL);
}

int main(int argc,char *argv[]){
	int loginworked = FALSE;
	char strporta[100];

	if (argc!=4){
		printf("Escreva no formato: ./dropboxClient <ID_do_usuário> <endereço_do_host> <porta>\n");
	}
	else{ //wrote correcly the arguments
		strcpy(userID,argv[1]);
		strcpy(host,argv[2]);
		strcpy(strporta,argv[3]);
		port = atoi(strporta);
		server = gethostbyname(host);


		if ((socket_local = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
			printf("ERROR opening socket");
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
		bzero(&(serv_addr.sin_zero), 8);

		printf("Socket pelo qual o cliente está enviando coisas: %d\n",socket_local);


		loginworked = login_server(host,port);
		if(!loginworked){
			printf("Login não funcionou!");
		}
		else{
			pthread_t tid[2];
	    int i = 0;
	    int n_threads = 2;
			double last_time;
			double actual_time;
			last_time=  (double) clock() / CLOCKS_PER_SEC;
			actual_time = (double) clock() / CLOCKS_PER_SEC;
			pthread_create(&(tid[0]), NULL, thread_interface,NULL);

			while(!mustexit){ //exits here when the user digits 'quit' at the interface thread
				actual_time = (double) clock() / CLOCKS_PER_SEC;

				if ((!is_syncing)&&(actual_time - last_time >= time_between_sync)){ //throws a sync thread every 10 sec, if there isn't one already
					last_time = actual_time;
					pthread_create(&(tid[1]), NULL, thread_sync, NULL);
				}
			}
			pthread_join(tid[0], NULL);
			//close_session();
		}
	}

	return 0;
}

#endif
