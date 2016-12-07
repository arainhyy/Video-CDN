//
// Created by yayunh on 12/5/16.
//

#include "load_balance.h"
extern node* clients;
extern int client_num;
extern node* servers;
extern int server_num;
extern node* nodes;
extern int total_num;

routing_table_entry* routing_table;

/**
 * Build routing table according to parsed topo.
 *
 * @return 0 if build routing table successfully.
 */
int build_routing_table() {
  // Precondition.
  if (client_num <= 0 || server_num <= 0 || total_num <= 0 || nodes == NULL || servers == NULL || clients == NULL) {
    return 1;
  }
  routing_table = (routing_table_entry*) malloc(sizeof(routing_table_entry) * client_num);

  node* client = nodes;
  int cnt = 0;
  while (client) {
    if (exist_in_list(client->ip, &servers) || client->ip[0] == 'r' || client->neighbor_num == 0) {
      client = client->next;
      continue;
    }
    queue_node header;
    memset(&header, sizeof(queue_node), 0);
    queue_node tail;
    memset(&tail, sizeof(queue_node), 0);
    header.next = &tail;
    tail.next = NULL;
    header.prev = NULL;
    tail.prev = &header;
    strcpy(routing_table[cnt].client_ip, client->ip);
    char* server_ip = bfs(&client, &header, &tail);
    strcpy(routing_table[cnt++].server_ip, server_ip);
    client = client->next;
  }
  return 0;
}

/**
 * Use BFS algorithm to get ip of nearest server of a client node.
 *
 * @param _now Address of client node.
 * @param header Header of doubly queue struct.
 * @param tail Tail of doubly queue struct.
 * @return IP of nearest server of this client node.
 */
char* bfs(node** _now, queue_node* header, queue_node* tail) {
  // Init visited state of each node.
  node* pt = nodes;
  while (pt != NULL) {
    pt->visited = 0;
    pt = pt->next;
  }
  // Start BFS with target node.
  queue_node* now = (queue_node*) malloc(sizeof(queue_node));
  now->n = *_now;
  now->n->visited = 1;
  push_back(tail, now);
  while (!is_empty(header, tail)) {
    now = pop_front(header, tail);
    int i = now->n->neighbor_num - 1;
    for (; i >= 0; i--) {
      if (!now->n->neighbors[i]->visited) {
        if (exist_in_list(now->n->neighbors[i]->ip, &servers)) {
          return now->n->neighbors[i]->ip;
        }
        queue_node* temp = (queue_node*) malloc(sizeof(queue_node));
        temp->n = now->n->neighbors[i];
        temp->n->visited = 1;
        push_back(tail, temp);
      }
    }
  }
  return NULL;
}

/**
 * Push back a queue node into this doubly linked list.
 *
 * @param tail Tail of doubly queue struct.
 * @param new_node New node to be pushed back.
 */
void push_back(queue_node* tail, queue_node* new_node) {
  new_node->next = tail;
  new_node->prev = tail->prev;
  tail->prev->next = new_node;
  tail->prev = new_node;
}

/**
 * Pop the first element of this doubly linked list.
 *
 * @param header Header of doubly queue struct.
 * @param tail Tail of doubly queue struct.
 * @return Node to be poped.
 */
queue_node* pop_front(queue_node* header, queue_node* tail) {
  queue_node* node = header->next;
  header->next = header->next->next;
  header->next->prev = header;
  return node;
}

/**
 * Check if the doubly linked list is empty.
 *
 * @param header Header of doubly queue struct.
 * @param tail Tail of doubly queue struct.
 * @return Boolean to indicate if this list is empty.
 */
int is_empty(queue_node* header, queue_node* tail) {
  if (header->next == tail || tail->prev == header) return 1;
  return 0;
}