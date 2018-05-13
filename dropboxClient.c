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

#define SOCKET int
#define TRUE 1
#define FALSE 0
int is_syncing = FALSE;
int mustexit = FALSE;
char userID[20];
char host[20];
int port;

int login_server(char *host,int port){
	int sockfd, n;
	unsigned int length;
	struct sockaddr_in serv_addr, from;
	struct hostent *server;
	char buffer[1250];
	strcpy(buffer,"logins0000");
	strcat(buffer,userID);
	server = gethostbyname(host);
	if (server == NULL) {
				fprintf(stderr,"ERROR, no such host\n");
				exit(0);
		}
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);
	n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0)
		printf("ERROR sendto");
	length = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		printf("ERROR recvfrom");
	printf("Got an ack: %s\n", buffer);
	close(sockfd);
	return 1;
}

void sync_client(){
	int x = 2+2;


}

void send_file(char *file){
	int x = 2+2;

}

void get_file(char *file){
	int x = 2+2;


}

void delete_file(char *file){
	int x = 2+2;

}

void close_session(){
	int x = 2+2;

}

char * getArgument(char command[100]){
	char* argument;
	int i=0; int j=0;
	argument = (char*) malloc(sizeof(char)*100);

	while(command[i]!=' ')
		i++;
	while(command[i]==' ')
		i++;

	while((command[i]!=' ')||(command[i]!='\0')){
		argument[j]= command[i];
		i++;
		j++;
	}

	return argument;

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
		//printf("funcao 4\n");
		result = 3;
	}
	else if (!strncmp("list_client",command,11)){
		//printf("funcao 5\n");
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
