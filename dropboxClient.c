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


#define SOCKET int
#define TRUE 1
#define FALSE 0

int is_syncing = FALSE;
int mustexit = FALSE;




int login_server(char *host,int port){

return 0;
}
void sync_client(){



}
void send_file(char *file){


}
void get_file(char *file){



}
void delete_file(char *file){



}
void close_session(){




}

void treat_command(char command[100]){
	if (!strncmp("exit",command,4)){
		mustexit = TRUE;
	}
	else if (!strncmp("upload",command,6)){
		//printf("funcao 2\n");
	}
	else if (!strncmp("download",command,8)){
		//printf("funcao 3\n");
	}
	else if (!strncmp("list_server",command,11)){
		//printf("funcao 4\n");
	}
	else if (!strncmp("list_client",command,11)){
		//printf("funcao 5\n");
	}
	else if (!strncmp("get_sync_dir",command,12)){
		//printf("funcao 6\n");
	}


	//printf("Seu comando foi: %s \n",command);
}


void *thread_sync(void *vargp)
{
		is_syncing = TRUE;


    //printf("\nSincronizando a pasta local...");

		is_syncing = FALSE;
    pthread_exit((void *)NULL);
}
void *thread_interface(void *vargp)
{
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

	if (argc!=4){
		printf("Escreve no formato: ./dropboxClient <ID_do_usuário> <endereço_do_host> <porta>\n");
	}
	else{

		char userID[100];
		char host[100];
		char porta[100];

		strcpy(userID,argv[1]);
		strcpy(host,argv[2]);
		strcpy(porta,argv[3]);
		//login_server(char *host,int port);



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

		return 0;
}




#endif
