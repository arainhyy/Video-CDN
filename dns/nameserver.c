//
// Created by yayunh on 11/22/16.
//

#include "nameserver.h"

extern node* clients;
extern int client_num;
extern node* servers;
extern int server_num;
extern node* nodes;
extern int total_num;

extern routing_table_entry* routing_table;

struct nameserver_param {
  int is_round_robin;
  char log_file[PATH_LEN];
  int listen_port;
  char ip[PATH_LEN];
  char server_file[PATH_LEN];
  char lsa_file[PATH_LEN];
} config;

fd_set ready;
fd_set read_set;
int fd_max;

static int nameserver_setup_listen() {
  FD_ZERO(&ready);
  // create socket ipv4 on UDP protocol.
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("proxy_setup_listen socket");
    return -1;
  }
  // set sock for incoming connections
  FD_SET(sock, &ready);
  fd_max = sock + 1;
  // initialize address
  struct sockaddr_in nameserver_addr;
  nameserver_addr.sin_family = AF_INET;
  inet_pton(AF_INET, config.ip, &(nameserver_addr.sin_addr));
  nameserver_addr.sin_port = htons(config.listen_port);
  // bind to address
  if (bind(sock, (struct sockaddr *) (&nameserver_addr), sizeof(struct sockaddr_in)) < 0) {
    perror("nameserver_setup_listen bind");
    return -1;
  }
  // listen to port
  if (listen(sock, PROXY_MAX_LISTEN) < 0) {
    perror("nameserver_setup_listen listen");
    return -1;
  }
  return sock;
}

int main(int argc, char* argv[]) {
  if (argc != 6 && argc != 7) {
    fprintf(stderr, "Usage: ./nameserver [-r] <log> <ip> <port> <servers> <LSAs>\n");
    return EXIT_FAILURE;
  }
  config.is_round_robin = argc == 6 ? 0 : 1;
  int argv_offset = config.is_round_robin;
  strcpy(config.log_file, argv[argv_offset + 1]);
  strcpy(config.ip, argv[argv_offset + 2]);
  config.listen_port = atoi(argv[argv_offset + 3]);
  strcpy(config.server_file, argv[argv_offset + 4]);
  strcpy(config.lsa_file, argv[argv_offset + 5]);

  start_logger("mylog.txt");
  nameserver_setup_listen();
  // Parse two files.
  int ret = 0;
  ret += parse_server_file(config.server_file);
  ret += parse_LSA_file(config.lsa_file);
  ret += build_routing_table();
  if (ret > 0) {
    fprintf(stderr, "Parse file error\n");
  }

  // TODO(hanlins): use server_ip to reply request from client.
  //************************************************************
  char* server_ip;
  char* client_ip; // Get this from deserialized packet.
  node* round_robin_pt = servers;
  if (config.is_round_robin) {
    server_ip = round_robin_pt->ip;
    round_robin_pt = round_robin_pt->next;
    if (!round_robin_pt) {
      round_robin_pt = servers;
    }
  } else {
    int i;
    for (i = 0; i < client_num; i++) {
      if (strcmp(routing_table[i].client_ip, client_ip)) {
        server_ip = routing_table[i].server_ip;
        break;
      }
    }
  }
  //************************************************************

  close_logger();
  return 0;
}