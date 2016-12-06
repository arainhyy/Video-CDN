//
// Created by yayunh on 12/5/16.
//

#ifndef TEST_PARSE_H
#define TEST_PARSE_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_IP_LEN 36
#define MAX_LINE_LEN 4096
#define ALREADY_EXISTED 0
#define NOT_ALREADY_EXISTED 1

typedef struct _node {
  char ip[MAX_IP_LEN];
  struct _node* next;
} node;

node* clients;
node* servers;
node* nodes;

int parse_LSA_file(char* file_name);
int parse_server_file(char* file_name);
int add_node_by_ip(char* ip, node** _header);
int exist_in_list(char* ip, node** _header);
#endif //TEST_PARSE_H
