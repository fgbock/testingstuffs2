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

#define SOCKET int
#define TRUE 1
#define FALSE 0
#define BUFFERSIZE 1250

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int is_syncing = FALSE;
int mustexit = FALSE;
char userID[20];
char host[20];
int port;
int socket_local;
struct sockaddr_in serv_addr;
struct hostent *server;

//=======================================================
int login_server(char *host,int port){
	int n;
	unsigned int length;
	struct sockaddr_in from;
	char buffer[BUFFERSIZE];
	char ackesperado[100];
	char bufferack[100];
	int i;
	int recebeuack = FALSE;

	create_home_dir(userID);

	for (i=0;i<BUFFERSIZE;i++)
		buffer[i]='\0';

	strcpy(ackesperado,"ACKlogins0000");
	strcpy(buffer,"logins0000");
	strcat(buffer,userID);

	length = sizeof(struct sockaddr_in);

	while(!recebeuack){
		n = sendto(socket_local, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, bufferack, 100, 0, (struct sockaddr *) &from, &length);
		bufferack[13] = '\0'; //wtf
		if (strcmp(ackesperado,bufferack)==0){
			recebeuack = TRUE;
		}
	}


	return 1;
}

void send_file(char *file){
	int n;
	unsigned int length;
	struct sockaddr_in from;
	struct hostent *server;
	char buffer[256];
	char ackesperado[100];
	char bufferack[100];
	int recebeuack = FALSE;
	struct sockaddr* destiny;

	strcpy(ackesperado,"ACKupload0000");
	strcpy(buffer,"upload0000");
	strcat(buffer,userID);
	strcat(buffer," ");
	strcat(buffer,file);

	length = sizeof(struct sockaddr_in);

	while(!recebeuack){
		n = sendto(socket_local, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, bufferack, 100, 0, (struct sockaddr *) &from, &length);
		bufferack[13] = '\0';
		printf("Ack recebido: %s\n",bufferack);
		if (!strcmp(ackesperado,bufferack)){
			recebeuack = TRUE;
		}
	}
	recebeuack = send_file_to(socket_local, file, *((struct sockaddr*) &serv_addr));

}

void get_file(char *file){
	int n;
	unsigned int length;
	struct sockaddr_in  from;
	struct hostent *server;
	char buffer[256];
	char ackesperado[100];
	char bufferack[100];
	int recebeuack = FALSE;

	strcpy(ackesperado,"ACKdownlo0000");
	strcpy(buffer,"downlo0000");
	strcat(buffer,file);

	length = sizeof(struct sockaddr_in);

	while(!recebeuack){
		n = sendto(socket_local, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, bufferack, 100, 0, (struct sockaddr *) &from, &length);
		if (!strcmp(ackesperado,bufferack)){
			recebeuack = TRUE;
		}
	}

	recebeuack = receive_file_from(socket_local, file,  *((struct sockaddr*) &serv_addr));

}

void delete_file(char *file){
	int n;
	unsigned int length;
	struct sockaddr_in from;
	struct hostent *server;
	char buffer[256];
	char ackesperado[100];
	char bufferack[100];
	int recebeuack = FALSE;
	int ret;
	FILE *fp;

	fp = fopen (file,"r");
  if (fp == NULL) {
      printf ("Arquivo não existe\n");
  }
	else{
		strcpy(ackesperado,"ACKdelete0000");
		strcpy(buffer,"delete0000");
		strcat(buffer,file);

		length = sizeof(struct sockaddr_in);
		while(!recebeuack){
			n = sendto(socket_local, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			n = recvfrom(socket_local, bufferack, 100, 0, (struct sockaddr *) &from, &length);
			if (!strcmp(ackesperado,bufferack)){
				recebeuack = TRUE;
			}
		}
	}
	ret = remove(file);

	if(ret == 0) {
		 printf("Arquivo foi deletado com sucesso!\n");
	} else {
		 printf("Algo deu errado...\n");
	}

}

void sync_client(){
	char path[256];
	strcpy(path, "~/sync_dir_");
	strcat(path, userID);
	int length, i = 0;
	int fd;
	int wd;
	char buffer[BUF_LEN];
	fd = inotify_init();
	wd = inotify_add_watch(fd,path,IN_MODIFY | IN_CREATE | IN_DELETE );
	length = read( fd, buffer, BUF_LEN );

	if ( length < 0 ) {
		perror( "read" );
	}

	while ( i < length ) {
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
		if ( event->len ) {
			if ( event->mask & IN_CREATE ) {
				if ( event->mask & IN_ISDIR ) {
					//printf( "The directory %s was created.\n", event->name );
					strcat(path,event->name);
					send_file(path);
				}
				else {
					//printf( "The file %s was created.\n", event->name );
				}
			}
			else if ( event->mask & IN_DELETE ) {
				if ( event->mask & IN_ISDIR ) {
					//printf( "The directory %s was deleted.\n", event->name );
				}
				else {
					//printf( "The file %s was deleted.\n", event->name );
					strcat(path,event->name);
					delete_file(path);
				}
			}
			else if ( event->mask & IN_MODIFY ) {
				if ( event->mask & IN_ISDIR ) {
					//printf( "The directory %s was modified.\n", event->name );
				}
				else {
					//printf( "The file %s was modified.\n", event->name );
					strcat(path,event->name);
					send_file(path);
				}
			}
		}
		i += EVENT_SIZE + event->len;
	}

	( void ) inotify_rm_watch( fd, wd );
	( void ) close( fd );


}

void close_session(){
	int n;
	unsigned int length;
	struct sockaddr_in  from;
	struct hostent *server;
	char buffer[100];
	char ackesperado[100];
	char bufferack[100];
	int recebeuack = FALSE;

	strcpy(ackesperado,"ACKcloses0000");
	strcpy(buffer,"closes0000");

	length = sizeof(struct sockaddr_in);

	while(!recebeuack){
		n = sendto(socket_local, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, bufferack, 100, 0, (struct sockaddr *) &from, &length);
		if (!strcmp(ackesperado,bufferack)){
			recebeuack = TRUE;
		}
	}
}
//=======================================================
void list_server(){
	int n;
	unsigned int length;
	struct sockaddr_in  from;
	struct hostent *server;
	char buffer[256];
	char ackesperado[100];
	char bufferack[100];
	int recebeuack = FALSE;
	int i,j;

	strcpy(ackesperado,"ACKlist_f0000");
	strcpy(buffer,"list_f0000");
	length = sizeof(struct sockaddr_in);

	while(!recebeuack){
		n = sendto(socket_local, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		n = recvfrom(socket_local, bufferack, 100, 0, (struct sockaddr *) &from, &length);
		if (!strncmp(ackesperado,bufferack,13)){
			recebeuack = TRUE;
		}
	}

	i = 0; j=13;
	while(bufferack[j]!= '\0'){
		buffer[i]=bufferack[j];
		i++;
		j++;
	}
	printf("%s",buffer);
}

void list_client(){

  DIR *dp;
  struct dirent *ep;
	char saida[100];
	char path[256];

	strcpy(path, "~/sync_dir_");
	strcat(path, userID);

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
		get_file(argument);

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
		if (must_sync){
			sync_client();
		}

    //printf("\nSincronizando a pasta local...");

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

				if ((!is_syncing)&&(actual_time - last_time >= 10.f)){ //throws a sync thread every 10 sec, if there isn't one already
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
