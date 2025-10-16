#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "requests.h"
#include <sys/time.h>

#define PORT 6767
#define MAX_BUFFER 8192

int main() {
	printf("Hello world!\n");
	FILE* file = fopen("index.html", "r");
	char buffer[MAX_BUFFER];
	char response[MAX_BUFFER]; 
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 250000; 

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
		int keepAlive = 1;
		int newSocket = accept(mySocket, (struct sockaddr*) &newAddr, &newAddrSize);
		setsockopt(newSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		char filepath[64];
		FILE* file;
		long filesize;
		if (newSocket < 0) {
			perror("error accepting new socket\n");
			continue;
		}
		printf("\n-----new connection!-----\n");
		while (keepAlive) {
			int newRead = read(newSocket, buffer, MAX_BUFFER - 1);
			char* reqEnd = strstr(buffer, "\r\n\r\n");
			if (newRead <= 0) {
				printf("Client disconnected\n");
				break;
			} else {
				buffer[newRead] = '\0';
				fputs(buffer, stdout);
			} 
			while (reqEnd == NULL) {
				int bufferlen = strlen(buffer);
				int newRead = read(newSocket, buffer + bufferlen, MAX_BUFFER - 1 - bufferlen);
				if (newRead > 0) {
					buffer[newRead] = '\0';
					fputs(buffer, stdout);
				}
				reqEnd = strstr(buffer, "\r\n\r\n");
			}
			printf("Request loop start (Bytes read: %d)\n", newRead);
			int headerEval = parseHeaders(buffer);
			if (headerEval == -1) {
				makeHeader(newSocket, "BADREQ", 0);
				break;
			} else if (headerEval == 0) {
				keepAlive = 0;
			}
			switch (parseRequest(buffer, filepath, 64)) {
				case 1: //GET
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
			printf("Request loop complete.\n\n");
		}
		close(newSocket);
	}
	return 0;
}
