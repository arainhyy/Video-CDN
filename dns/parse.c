//
// Created by yayunh on 12/5/16.
//

#include "parse.h"

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
  int seq_num;
  char neighbors[MAX_LINE_LEN];
  while (fgets(line, MAX_LINE_LEN, f)) {
    if (sscanf(line, "%s, %d %s", node_ip, &seq_num, neighbors) < 3) {
      return -1;
    }
    add_node_by_ip(node_ip, &nodes);
    if (exist_in_list(node_ip, &servers) == NOT_ALREADY_EXISTED) {
      add_node_by_ip(node_ip, &clients);
    }
  }
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
    add_node_by_ip(node_ip, &nodes);
    add_node_by_ip(node_ip, &servers);
  }
  return 0;
}

/**
 * Add node to list providing IP field value.
 *
 * @param ip IP field value.
 * @param _header Address of list header.
 * @return 1 if add successfully.
 */
int add_node_by_ip(char* ip, node** _header) {
  if (exist_in_list(ip, _header) == ALREADY_EXISTED) {
    return ALREADY_EXISTED;
  }
  node* pt = *_header;
  node* new_node = (node*) malloc(sizeof(node));
  strcpy(new_node->ip, ip);
  new_node->next = pt;
  _header = &new_node;
  return 1;
}

/**
 * Check if this node has already existed in node list by ip field.
 * @param ip IP field value.
 * @param _header Address of list header.
 * @return ALREADY_EXISTED or NOT_ALREADY_EXISTED.
 */
int exist_in_list(char* ip, node** _header) {
  node* pt = *_header;
  while (pt != NULL) {
    // If this node has been added in list, return.
    if (strcmp(ip, pt->ip) == 0) {
      return ALREADY_EXISTED;
    }
    pt = pt->next;
  }
  return NOT_ALREADY_EXISTED;
}