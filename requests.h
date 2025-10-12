#ifndef REQUESTS_H
#define REQUESTS_H
#include <stdio.h>

void readFile(FILE* file, char* buffer);

void parseRequest(char* request, char* response);

void getMIMEType(char* filename, char* request);

void makeHeader(char* response, char* filetype);
#endif
