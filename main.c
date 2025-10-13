#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "requests.h"

#define PORT 6767
#define MAX_BUFFER 8192

int main() {
	printf("Hello world!\n");
	FILE* file = fopen("index.html", "r");
	char buffer[MAX_BUFFER];
	char response[MAX_BUFFER]; 

	struct sockaddr_in myAddr;
	int mySocket = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //allows quick restart without "address already in use"

	myAddr.sin_family = AF_INET; //ipv4 family
	myAddr.sin_addr.s_addr = INADDR_ANY; //bind to every ip of the local computer
	myAddr.sin_port = htons(PORT);
	int addrSize = sizeof(myAddr);

	if (bind(mySocket, (struct sockaddr*) &myAddr, sizeof(myAddr)) != 0) {
		perror("error binding \n");
		return -1;
	}
	listen(mySocket, 20);
	printf("Now listening on port %d...\n", PORT);
	while(1) { //request loop
		struct sockaddr_in newAddr;
		socklen_t newAddrSize = sizeof(newAddr);
		int newSocket = accept(mySocket, (struct sockaddr*) &newAddr, &newAddrSize);
		char filepath[64];
		FILE* file;
		long filesize;
		if (newSocket < 0) {
			perror("error accepting new socket\n");
			continue;
		}
		printf("new connection!\n");
		int newRead = read(newSocket, buffer, MAX_BUFFER - 1);
		if (newRead < 0) {
			perror("read failed\n");
			return -1;
		} else {
			buffer[newRead] = '\0';
			fputs(buffer, stdout);
		} 
		switch (parseRequest(buffer, filepath, 64)) {
			case 0:
				file = fopen("index.html", "rb");
				if (file == NULL) {
					makeHeader(newSocket, "DNE", 0);
				} else {
					fseek(file, 0, SEEK_END);
					filesize = ftell(file);
					rewind(file);
					makeHeader(newSocket, "HOME", filesize);
					readFile(file, 0, filesize, newSocket);
					fclose(file);
				}
				break;
			case 1:
				file = fopen(filepath, "rb");
				if (file == NULL) {
					makeHeader(newSocket, "DNE", 0);
				} else {
					fseek(file, 0, SEEK_END);
					filesize = ftell(file);
					int chunked = filesize > MAX_BUFFER;
					rewind(file);
					char* extension = getMIMEType(filepath);
					makeHeader(newSocket, extension, filesize);
					if (strcmp(extension, "DNE") != 0 && strcmp(extension, "FORBID") != 0) {
						readFile(file, chunked, filesize, newSocket);
					}
					free(extension);
					fclose(file);
				}
				break;
			default:
			break;
		}
		close(newSocket);
	}
	return 0;
}
