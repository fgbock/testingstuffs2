#include <sys/socket.h>
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

struct packet {
	short int opcode;
	short int seqnum;
	char data [PACKETSIZE - 4];
};

struct sockaddr_in serv_addr;
struct hostent *server;
char host[20];
int port;
int socket_local;
char userID[20];

int main(int argc,char *argv[]){
  char strporta[100];
  struct packet request;
  struct sockaddr_in from;
  int n, length;

	if (argc!=4){
		printf("Escreva no formato: ./tester <ID_do_usuário> <endereço_do_host> <porta>\n");
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
  }

  // Send a login request
  request.opcode = LOGIN;
  strncpy(request.data,userID,20);
  printf("\nopcode is %hi\n\n",request.opcode);
  printf("\ndata is %s\n\n",request.data);
  n = sendto(socket_local, (char *) &request, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
  n = recvfrom(socket_local, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
  printf("\nlogin reply is %hi\n\n",request.opcode);
  printf("\nnew port is is %hi\n\n",request.seqnum);
  sleep(2);
  // Send an upload request

  // Send a close request

}