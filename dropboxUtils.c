//#ifndef DROPBOXUTILS_C
//#define DROPBOXUTILS_C
#include"dropboxUtils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm/errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>

#define TRUE 1
#define FALSE 0

struct file_info{
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
};

struct client{
	int devices[2];
	char userid[MAXNAME];
	struct file_info fi[MAXFILES];
	int logged_in;
};

int create_home_dir(char *userID){
	//cria diretório com nome do user no HOME do user, chamado pelo cliente
	char *path = malloc((strlen(userID)+15)*sizeof(char));
	//strcpy(dir, "mkdir ~/");
	strcpy(path, "~/sync_dir_");
	strcat(path, userID);
	DIR *dir = opendir(path);
	int ret=0;
	if(dir){
		closedir(dir); //diretório já existe, apenas fechamos ele
	}else if(ENOENT==errno){ //diretório não existe!!
		char *syscmd = malloc((strlen(userID)+20)*sizeof(char));
		strcpy(syscmd, "mkdir ");
		strcat(syscmd, path);
		ret = system(syscmd);
		free(syscmd);
	}
	free(path);
	return ret;
}

int create_home_dir_server(char *userID){
	//cria diretório com nome do user no HOME do user, chamado pelo cliente
	char path[100];
	//strcpy(dir, "mkdir ~/");
	strcpy(path, "~/dropboxserver/sync_dir_");
	strcat(path, userID);
	DIR *dir = opendir(path);
	int ret=0;
	if(dir){
		closedir(dir); //diretório já existe, apenas fechamos ele
	}else if(ENOENT==errno){ //diretório não existe!!
		char syscmd[200];
		strcpy(syscmd, "mkdir ");
		strcat(syscmd, path);
		ret = system(syscmd);
		//free(syscmd);
	}
	//free(path);
	return ret;
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

int create_server_root(){
	//cria diretório raiz do servidor na home da máquina, caso não exista ainda
	char *path = malloc(15*sizeof(char));
	strcpy(path, "~/dropboxserver");
	DIR *dir = opendir(path);
	int ret=0;
	if(dir){
		closedir(dir); //diretório já existe, apenas fechamos ele
	}else if(ENOENT==errno){ //diretório não existe!!
		char *syscmd = malloc(20*sizeof(char));
		strcpy(syscmd, "mkdir ");
		strcat(syscmd, path);
		ret = system(syscmd);
		free(syscmd);
	}
	free(path);
	return ret;
}

int create_server_userdir(char *userID){
	//cria diretório com nome do user no na raiz do servidor
	char *path = malloc((strlen(userID)+17)*sizeof(char));
	strcpy(path, "~/dropboxserver/");
	strcat(path, userID);
	DIR *dir = opendir(path);
	int ret=0;
	if(dir){
		closedir(dir); //diretório já existe, apenas fechamos ele
	}else if(ENOENT==errno){ //diretório não existe!!
		char *syscmd = malloc((strlen(userID)+25)*sizeof(char));
		strcpy(syscmd, "mkdir ");
		strcat(syscmd, path);
		ret = system(syscmd);
		free(syscmd);
	}
	free(path);
	return ret;
}


int receive_int_from(int socket){
	//retorn número de op recebido no socket socket, ou -1(op inválida) caso erro
	int n;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	clilen = sizeof(struct sockaddr_in);

	//char *buf = malloc(sizeof(char)); //recebe um char apenas representando um número de op no intervalo [1...algo]
	int receivedInt = 0;

	n = recvfrom(socket,(char*) &receivedInt, sizeof(int), 0, (struct sockaddr *) &cli_addr, &clilen);
	if (n < 0)
		return -1;

	//retorna o ack
	n = sendto(socket, "ACK", 3, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
	if (n  < 0)
		return -1;

	//int ret = atoi(buf);
	//free(buf);
	return ntohl(receivedInt);
}

int send_int_to(int socket, int op){
	//envia uma op como um inteiro, -1 se falha e  0  se sucesso
	int n;
	struct sockaddr_in serv_addr, from;
	//char *buf = malloc(sizeof(char)); //op tem apenas um char/dígito
	//sprintf(buf, "%d", op); //conversão de op para a string buf

	int sendInt = htonl(op);

	n = sendto(socket, (char*) &sendInt, sizeof(int), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0)
		return -1;

	//free(buf);
	char *buf = malloc(sizeof(char)*3); //para receber o ack

	unsigned int length = sizeof(struct sockaddr_in);
	n = recvfrom(socket, buf, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		return -1;

	free(buf);

	return 0;
}


char* receive_string_from(int socket){
	int str_size = receive_int_from(socket); //recebe o tamamho do string a ser lido

	char *buf = malloc(sizeof(char)*str_size);

	int n;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	clilen = sizeof(struct sockaddr_in);

	n = recvfrom(socket,buf, str_size, 0, (struct sockaddr *) &cli_addr, &clilen);
	if (n < 0)
		return NULL;

	//retorna o ack
	n = sendto(socket, "ACK", 3, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
	if (n  < 0)
		return NULL;

	return buf;
}

int send_string_to(int socket, char* str){
	int str_size = strlen(str);

	send_int_to(socket, str_size); //envia o tam do string a ser lido pro server

	int n;
	struct sockaddr_in serv_addr, from;

	n = sendto(socket, str, str_size, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0)
		return -1;

	char *buf = malloc(sizeof(char)*3); //para receber o ack

	unsigned int length = sizeof(struct sockaddr_in);
	n = recvfrom(socket, buf, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		return -1;

	free(buf);

	return 0;

}


int receive_file_from(int socket, char* file_name){
	int n, file;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	char buf[CHUNK];
	clilen = sizeof(struct sockaddr_in);
	int recebeutudo = FALSE;
	char *bufACK = malloc(sizeof(char)*3); //para receber o ack
	int counter = 0;
	char mensagemdeconfirmacao[100];
	char mensagemdeconfirmacaoanterior[100];
	char mensagemesperada[100];
	struct sockaddr_in serv_addr, from;
	char bufferitoa[100];

	file = open(file_name, O_RDWR | O_CREAT, 0666);

	while(!recebeutudo){
		strcpy(mensagemesperada,"");
		strcat(mensagemesperada,"packet");
		sprintf(bufferitoa,"%d",counter);
		strcat(mensagemesperada,bufferitoa); //mensagem de confirmacao é ACKpacket<numerodopacote>
		strcpy(mensagemdeconfirmacao,"");
		strcat(mensagemdeconfirmacao,"ACKpacket");
		strcat(mensagemdeconfirmacao,bufferitoa); //mensagem de confirmacao é ACKpacket<numerodopacote>
		strcpy(mensagemdeconfirmacaoanterior,"");
		strcat(mensagemdeconfirmacaoanterior,"ACKpacket");
		sprintf(bufferitoa,"%d",(counter-1));
		strcat(mensagemdeconfirmacaoanterior,bufferitoa); //mensagem de confirmacao é ACKpacket<numerodopacote>

		n = recvfrom(socket, buf, CHUNK, 0, (struct sockaddr *) &cli_addr, &clilen);

		if(strcmp(buf, "xxxCABOOARQUIVOxxx")==0){ //se recebeu pacote de fiim de arquivo
			recebeutudo = TRUE;
			sendto(socket, "xxxCABOOARQUIVOxxx", n, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		}
		else{
			if(!strncmp(buf,mensagemesperada,6)){ //se recebeu o pacote correto
				write(file, buf, n);
				sendto(socket, mensagemdeconfirmacao, n, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			}
			else{ //se recebeu o pacote anterior
				sendto(socket, mensagemdeconfirmacaoanterior, n, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			}
		}

		counter++;
	}

	close(file);


	n = sendto(socket, "ACK", 3, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
	if (n  < 0)
		return -1;

	return 0;

}

int send_file_to(int socket, char* file_name){
	int n, file,counter;
	char buf[CHUNK];
	char bufTrue[CHUNK+OPCODE];
	char mensagemdeconfirmacao[100];
	struct sockaddr_in serv_addr, from;
	char *bufACK = malloc(sizeof(char)*3); //para receber o ack
	counter = 0;
	unsigned int length = sizeof(struct sockaddr_in);
	file = open(file_name, O_RDONLY);
	char bufferitoa[100];

	while((n=read(file, buf, CHUNK))>0){
		strcat(bufTrue,"packet");
		sprintf(bufferitoa,"%d",counter);
		strcat(bufTrue,bufferitoa); //pacote é packet<numerodopacote><DATACHUNK>
		strcat(bufTrue,buf);
		strcpy(mensagemdeconfirmacao,"");
		strcat(mensagemdeconfirmacao,"ACKpacket");
		strcat(mensagemdeconfirmacao,bufferitoa); //mensagem de confirmacao é ACKpacket<numerodopacote>

		while(strcmp(bufACK,mensagemdeconfirmacao)){ //enquanto nao forem iguais
			sendto(socket, bufTrue, n, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			n = recvfrom(socket, bufACK, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
			sleep(1);
		}
		counter++;
	}

	//envia o sinal de final do arquivo, de tal forma que não precisa indicar tam do arquivo
	sendto(socket, "xxxCABOOARQUIVOxxx", n, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));




	n = recvfrom(socket, bufACK, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		return -1;

	free(bufACK);

	close(file);

	return 0;
}



//#endif
