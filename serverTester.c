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
#define FILE 8
#define END 9

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
  if (request.opcode != ACK){
    printf("Login unsuccesful\n\n")
    return -1;
  }
  printf("Login reply is %hi\n\n",request.opcode);
  printf("New connection port is is %hi\n\n",request.seqnum);

  // Send an upload request
	request.opcode = DELETE;
	n = sendto(socket_local, (char *) &request, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	n = recvfrom(socket_local, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
	if (request.opcode != ACK){
		printf("Upload ack unsuccesful\n\n")
		return -1;
	}
	// Here will be the actual upload:

  // Send a download request
	request.opcode = DOWNLOAD;
	n = sendto(socket_local, (char *) &request, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	n = recvfrom(socket_local, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
	if (request.opcode != ACK){
		printf("Download ack unsuccesful\n\n")
		return -1;
	}
	// Here will be the actual download:

  // Send a delete request
	request.opcode = DELETE;
	n = sendto(socket_local, (char *) &request, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	n = recvfrom(socket_local, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
	if (request.opcode != ACK){
		printf("Delete ack unsuccesful\n\n")
		return -1;
	}

  // Send a list request
	request.opcode = CLOSE;
	n = sendto(socket_local, (char *) &request, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
  n = recvfrom(socket_local, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
	if (request.opcode != ACK){
		printf("List ack unsuccesful\n\n")
		return -1;
	}
	n = recvfrom(socket_local, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
	if (request.opcode != LIST){
		printf("List op unsuccesful\n\n")
		return -1;
	}
	printf("List of files is: %s\n\n",request.data);

	// Send a close request
	request.opcode = CLOSE;
	n = sendto(socket_local, (char *) &request, PACKETSIZE, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
  n = recvfrom(socket_local, (char *) &request, PACKETSIZE, 0, (struct sockaddr *) &from, &length);
	if (request.opcode != ACK){
		printf("Close unsuccesful\n\n")
		return -1;
	}
}
