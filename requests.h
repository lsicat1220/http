#ifndef REQUESTS_H
#define REQUESTS_H
#include <stdio.h>

int readFile(FILE* file, int chunked, int size, int socket);

int parseRequest(char* request, char* response, int pathsize);

char* getMIMEType(char* filename);

void makeHeader(int socket, char* filetype, long size);
#endif
