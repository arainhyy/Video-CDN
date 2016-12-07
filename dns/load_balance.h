//
// Created by yayunh on 12/5/16.
//

#ifndef TEST_LOAD_BALANCE_H
#define TEST_LOAD_BALANCE_H
#include "parse.h"

typedef struct _routing_table_entry {
  char client_ip[MAX_IP_LEN];
  char server_ip[MAX_IP_LEN];
} routing_table_entry;

typedef struct _queue_node {
  node* n;
  struct _queue_node* next;
  struct _queue_node* prev;
} queue_node;

void push_back(queue_node* tail, queue_node* new_node);
queue_node* pop_front(queue_node* header, queue_node* tail);
int is_empty(queue_node* header, queue_node* tail);
char* bfs(node** _now, queue_node* header, queue_node* tail);

int build_routing_table();
#endif //TEST_LOAD_BALANCE_H
