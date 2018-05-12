both: client server


client: dropboxUtils.o dropboxClient.o 
	gcc -o dropboxClient dropboxUtils.o dropboxClient.o 
server: dropboxUtils.o dropboxServer.o 
	gcc -o dropboxServer dropboxUtils.o dropboxServer.o
dropboxUtils.o: dropboxUtils.c
	gcc -c dropboxUtils.c
dropboxClient.o: dropboxClient.c
	gcc -c dropboxClient.c
dropboxServer.o: dropboxServer.c
	gcc -c dropboxServer.c

clean:
	rm dropboxClient.o dropboxServer.o dropboxUtils.o dropboxClient dropboxServer
