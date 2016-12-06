//
// Created by yayunh on 11/22/16.
//

#include "nameserver.h"

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
  //TODO(yayunh):Parse two files.


  close_logger();
  return 0;
}