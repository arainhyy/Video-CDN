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

typedef struct _node {
  char ip[MAX_IP_LEN];
  struct _node** neighbors;
  int neighbor_num;
  int seq_num;
  struct _node* next;
  int visited;
} node;

void init();
int parse_LSA_file(char* file_name);
void add_reverse_neighbor(node* n, node* neighbor);
int parse_server_file(char* file_name);
node* add_node_by_ip_with_num(char* ip, node** _header, int* num);
node* exist_in_list(char* ip, node** _header);
#endif //TEST_PARSE_H
