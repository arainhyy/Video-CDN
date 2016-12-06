//
// Created by yayunh on 12/5/16.
//

#include "parse.h"

node* clients;
int client_num;
node* servers;
int server_num;
node* nodes;
int total_num;

int count_char_num(char* str, char c);

/**
 * Initialize global variables.
 */
void init() {
  clients = NULL;
  client_num = 0;
  servers = NULL;
  server_num = 0;
  nodes = NULL;
  total_num = 0;
}

/**
 * Parse topo in lsa file into node list with their neighbors.
 *
 * @param file_name Name of lsa file.
 * @return 0 if parse successfully.
 */
int parse_LSA_file(char* file_name) {
  FILE* f = fopen(file_name, "r");
  if (!f) {
    return -1;
  }
  char line[MAX_LINE_LEN];
  char node_ip[MAX_IP_LEN];
  int seq_num = -1;
  char neighbors[MAX_LINE_LEN];
  while (fgets(line, MAX_LINE_LEN, f)) {
    if (sscanf(line, "%s %d %s", node_ip, &seq_num, neighbors) < 3) {
      return -1;
    }

    node* now = add_node_by_ip_with_num(node_ip, &nodes, &total_num);
    if (exist_in_list(node_ip, &servers) == NULL) {
      add_node_by_ip_with_num(node_ip, &clients, &client_num);
    }
    if (seq_num > now->seq_num) {
      now->seq_num = seq_num;
      free(now->neighbors);
      int neighbor_num = count_char_num(neighbors, ',') + 1;
      now->neighbors = (node**) malloc(sizeof(node*) * neighbor_num);
      now->neighbor_num = neighbor_num;
      char* neighbor = strtok(neighbors, ",");
      int i;
      for (i = 0; i < neighbor_num; i++) {
        node* this_neighbor = exist_in_list(neighbor, &nodes);
        if (this_neighbor == NULL) {
          this_neighbor = add_node_by_ip_with_num(neighbor, &nodes, &total_num);
          // If it can not find this node at this time, this node must be client because servers get parsed already.
          add_node_by_ip_with_num(node_ip, &clients, &client_num);
        }
        now->neighbors[i] = this_neighbor;
        neighbor = strtok(NULL, ",");
      }
    }
  }
  fclose(f);
  return 0;
}

/**
 * Parse server ip addresses in server file into server list and node list.
 *
 * @param file_name Name of server file which contains all server ip addresses.
 * @return 0 if parse successfully.
 */
int parse_server_file(char* file_name) {
  FILE* f = fopen(file_name, "r");
  if (!f) {
    return -1;
  }
  char node_ip[MAX_IP_LEN];
  while (fgets(node_ip, MAX_LINE_LEN, f)) {
    node_ip[strlen(node_ip) - 1] = '\0';
    add_node_by_ip_with_num(node_ip, &nodes, &total_num);
    add_node_by_ip_with_num(node_ip, &servers, &server_num);
  }
  fclose(f);
  return 0;
}

/**
 * Add node to list providing IP field value.
 *
 * @param ip IP field value.
 * @param _header Address of list header.
 * @param num Pointer of number.
 * @return Node pointer.
 */
node* add_node_by_ip_with_num(char* ip, node** _header, int* num) {
  node* new_node;
  if ((new_node = exist_in_list(ip, _header)) != NULL) {
    return new_node;
  }
  (*num)++;
  node* pt = *_header;
  new_node = (node*) malloc(sizeof(node));
  strcpy(new_node->ip, ip);
  new_node->neighbors = NULL;
  new_node->seq_num = -1;
  new_node->next = pt;
  new_node->visited = 0;
  *_header = new_node;
  return new_node;
}

/**
 * Count character appearance times in a char array.
 *
 * @param str
 * @param c
 * @return Appearance times.
 */
int count_char_num(char* str, char c) {
  int len = strlen(str);
  int i;
  int cnt = 0;
  for (i = 0; i < len; i++) {
    if (str[i] == c) {
      cnt++;
    }
  }
  return cnt;
}

/**
 * Check if this node has already existed in node list by ip field.
 *
 * @param ip IP field value.
 * @param _header Address of list header.
 * @return Node pointer.
 */
node* exist_in_list(char* ip, node** _header) {
  node* pt = *_header;
  while (pt != NULL) {
    // If this node has been added in list, return.
    if (strcmp(ip, pt->ip) == 0) {
      return pt;
    }
    pt = pt->next;
  }
  return NULL;
}