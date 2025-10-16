#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "requests.h"

#define MAX_BUFFER 8192

int readFile(FILE* file, int chunked, int size, int socket) {
	rewind(file); //making sure file is at the start
	char buffer[MAX_BUFFER];
	printf("Start file read (Chunked = %d):\n", chunked);
	if (chunked == 1) {
		long remainingBytes = size - ftell(file);
		while(remainingBytes > 4096) {
			buffer[0] = 0;
			snprintf(buffer, MAX_BUFFER, "1000\r\n");
			write(socket, buffer, strlen(buffer));
			fread(buffer, 4096, 1, file);
			write(socket, buffer, 4096);
			write(socket, "\r\n", 2);
			remainingBytes = size - ftell(file);
		}
		snprintf(buffer, MAX_BUFFER, "%lx\r\n", remainingBytes);
		write(socket, buffer, strlen(buffer));
		fread(buffer, remainingBytes, 1, file);
		write(socket, buffer, remainingBytes);
		write(socket, "\r\n", 2);
		write(socket, "0\r\n\r\n", 5);
		return 0;
	} else {
		if (size > MAX_BUFFER) {
			return -1;
		}
		buffer[0] = 0;
		fread(buffer, size, 1, file);
		write(socket, buffer, size);
		return 0;
	}
}
int parseRequest(char* request, char* filepath, int pathsize) {
	char* saveptr; //saves next starting point for strtok_r
	char* reqptr = strtok_r(request, " ", &saveptr); //strtok_r returns into one pointer
	printf("Parsing request...\n");
	if (reqptr == NULL) {
		return -1; //invalid request
	} else if (strcmp(reqptr, "GET") == 0) { 
		char* pathptr = strtok_r(NULL, " ", &saveptr);
		if (pathptr == NULL || pathptr[0] == 0) {
			return -1;
		} else if (strcmp(pathptr, "/") == 0) {
			snprintf(filepath, pathsize, "index.html");
		} else {
			snprintf(filepath, pathsize, "%s", pathptr[0] == '/' ? pathptr+1 : pathptr);
		}
		return 1; //get file
	} else {
		return -2; //unimplemented method
	}
}
int parseHeaders(char* request) {
	char* charptr = strchr(request, '\n') + 1;
	char* nextNL;
	int validhost = 0;
	int keepAlive = 1;
	printf("Starting header parse...\n");
	while (charptr != NULL && *charptr != '\r') {
		nextNL = strchr(charptr, '\n');
		*nextNL = 0;
		//printf("Header read: %s\n", charptr);
		int linelen = nextNL - charptr;
		if (strncasecmp(charptr, "host", 4) == 0) {
			validhost = 1;
		} else if (strncasecmp(charptr, "connection", 10) == 0) {
			char* valptr = strcasestr(charptr, "close");
			if (valptr != NULL) {
				keepAlive = 0;
			}
		}
		*nextNL = '\n';
		charptr = strchr(nextNL, '\n') + 1;
	}
	if (validhost != 1) {
		return -1;
	}
	return keepAlive;
}
char* getMIMEType (char* filename) {
	char workingDir[64];
	char fileDir[64];
	char* extension = malloc(10);
	getcwd(workingDir, 64);
	realpath(filename, fileDir);
	if (strncmp(workingDir, fileDir, strlen(workingDir)) != 0) {
		strcpy(extension, "FORBID");
	}
	char namecpy[strlen(filename) + 1];
	strcpy(namecpy, filename);
	char* extdot = strrchr(namecpy, '.');
	if (extdot == NULL) {
		strncpy(extension, "txt", 4);
	} else if (strcmp(filename, "index.html") == 0) {
		strncpy(extension, "HOME", 5);
	} else {
		strncpy(extension, extdot+1, 9);
	}
	return extension;
}
void makeHeader (int socket, char* filetype, long size) {
	char response[MAX_BUFFER];
	//handling errors
	if (strcmp(filetype, "DNE") == 0) {
		snprintf(response, MAX_BUFFER, "HTTP/1.1 404 Not Found\r\n\r\n");
		write(socket, response, strlen(response));
		return;
	} else if (strcmp(filetype, "FORBID") == 0) {
		snprintf(response, MAX_BUFFER, "HTTP/1.1 403 Forbidden\r\n\r\n");
		write(socket, response, strlen(response));
		return;
	} else if (strcmp(filetype, "BADREQ") == 0) {
		snprintf(response, MAX_BUFFER, "HTTP/1.1 400 Bad Request\r\n\r\n");
		write(socket, response, strlen(response));
		return;
	} else {
		snprintf(response, MAX_BUFFER, "HTTP/1.1 200 OK\r\n");
	}

	int len = strlen(response);
	if (strcmp(filetype, "HOME") == 0) {
		snprintf(response + len, MAX_BUFFER - len,
		   "Server: webserver-c\r\n"
		   "Content-type: text/html\r\n");
	} else if (strcmp(filetype, "html") == 0) {
		snprintf(response + len, MAX_BUFFER - len, "Content-type: text/html\r\n");
	} else if (strcmp(filetype, "css") == 0) {
		snprintf(response + len, MAX_BUFFER - len, "Content-type: text/css\r\n");
	} else if (strcmp(filetype, "ico") == 0) {
		snprintf(response + len, MAX_BUFFER - len, "image/vnd.microsoft.icon");
	} else if (0){
		//insert other filetypes here
	} 
	len = strlen(response);
	if (size > 0) {
		if (size < MAX_BUFFER) {
			snprintf(response + len, MAX_BUFFER - len, "Content-length: %ld\r\n\r\n", size);
		} else {
			snprintf(response + len, MAX_BUFFER - len, "Transfer-Encoding: chunked\r\n\r\n");
		}
	}
	printf("\n-----RESPONSE:-----\n%s", response);
	write(socket, response, strlen(response));
	return;
}
