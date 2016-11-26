#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 0
#define NEEDMORE -1
//Header field
struct Request_header
{
	char header_name[4096];
	char header_value[4096];
	struct Request_header* next;
};

//HTTP Request Header
typedef struct
{
	char http_version[50];
	char http_method[50];
	char http_uri[4096];

	struct Request_header *headers;
	int header_count;
	// Fill in these fields after parsing.
	char connection[50];
	int content_length; // For POST method.
	int content_readed; // For POST method.
	int status;
	int position;
	int is_cgi;
	char content_type[50];
	char query[1024];
	char* post_body;
} Request;

Request* parse(char *buffer, int size,int socketFd);
