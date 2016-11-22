//
// Created by yayunh on 11/22/16.
//

#include "proxy.h"

struct proxy_param {
  char log_file[PATH_LEN];
  float alpha;
  int listen_port;
  char fake_ip[IP_LEN];
  char dns_ip[IP_LEN];
  int dns_port;
  char www_ip[IP_LEN];
} proxy;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "No log file\n");
    return EXIT_FAILURE;
  }
  strcpy(proxy.log_file, argv[1]);
  start_logger(proxy.log_file);




  close_logger();
}