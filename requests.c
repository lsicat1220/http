#include <stdio.h>
#include <string.h>
#include "requests.h"

#define MAX_BUFFER 2048

void readFile(FILE* file, char* buffer) {
	char read[1028];
	int len = strlen(buffer);
	while(len < MAX_BUFFER && fgets(buffer + len, MAX_BUFFER - len, file)) {
		len = strlen(buffer);
	}
}
void parseRequest(char* request, char* response) {
	char* saveptr; //saves next starting point for strtok_r
	response[0] = 0;
	char* reqptr = strtok_r(request, " ", &saveptr); //strtok_r returns into one pointer
	if (reqptr == NULL || strcmp(reqptr, "GET") != 0) {
		return;
	}
	char* pathptr = strtok_r(NULL, " ", &saveptr);
	getMIMEType(pathptr, response);
	return;
}
void getMIMEType (char* filename, char* response) {
	response[0] = 0;
	if (strcmp(filename, "/") == 0) {
		snprintf(response, MAX_BUFFER,
		   "HTTP/1.0 200 OK\r\n"
		   "Server: webserver-c\r\n"
		   "Content-type: text/html\r\n\r\n");
		FILE* html = fopen("index.html", "r");
		if (html == NULL) {
			return;
		}
		readFile(html, response);
		fclose(html);
		return;
	}
	if (filename[0] == '/') {
		filename++;
	}
	char namecpy[strlen(filename) + 1];
	strcpy(namecpy, filename);
	char* extdot = strrchr(namecpy, '.');
	char* extension;
	if (extdot != NULL) {
		*extdot = 0;
		extension = extdot+1;
	}
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		makeHeader(response, "DNE");
	} else {
		makeHeader(response, extension ? extension : "txt");
		readFile(file, response);
		fclose(file);
	}
	return;
}
void makeHeader (char* response, char* filetype) {
	response[0] = 0;
	if (strcmp(filetype, "DNE") == 0) {
		snprintf(response, MAX_BUFFER, "HTTP/1.0 404 Not Found\r\n");
		return;
	}
	snprintf(response, MAX_BUFFER, "HTTP/1.0 200 OK\r\n");
	int len = strlen(response);
	if (strcmp(filetype, "css") == 0) {
		snprintf(response + len, MAX_BUFFER - len, "Content-type: text/css\r\n\r\n");
	} else if (0){
		//insert other filetypes here
	} 
	return;
}
