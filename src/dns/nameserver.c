//
// Created by yayunh on 11/22/16.
//
#include <time.h>
#include "nameserver.h"
#include "../dns_record.h"

extern node *clients;
extern int client_num;
extern node *servers;
extern int server_num;
extern node *nodes;
extern int total_num;

extern routing_table_entry *routing_table;

struct nameserver_param {
    int is_round_robin;
    char log_file[PATH_LEN];
    int listen_port;
    char ip[PATH_LEN];
    char server_file[PATH_LEN];
    char lsa_file[PATH_LEN];
} config;

fd_set ready;
int fd_max;

static int nameserver_setup_listen();

static int nameserver_run();

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
    bzero(&nameserver_addr, sizeof(nameserver_addr));
    nameserver_addr.sin_family = AF_INET;
    struct in_addr ns_inaddr;
    inet_aton(config.ip, &ns_inaddr);
    nameserver_addr.sin_addr = ns_inaddr;
    nameserver_addr.sin_port = htons(config.listen_port);
    // bind to address
    if (bind(sock, (struct sockaddr *) (&nameserver_addr), sizeof(struct sockaddr_in)) < 0) {
        perror("nameserver_setup_listen bind");
        return -1;
    }
    return sock;
}

int main(int argc, char *argv[]) {
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
    log_init(config.log_file);
    start_logger("mylog.txt");
    // run nameserver
    return nameserver_run();
}

int sendto_wrap(int sock, char *buf, size_t size, int flag, struct sockaddr *addr, socklen_t addr_len) {
    int offset = 0;
    while ((size - offset) > 0) {
        int ret = sendto(sock, buf + offset, size - offset, flag, addr, addr_len);
        if (ret < 0) {
            perror("sendto");
            return -1;
        }
        offset += ret;
    }
    return offset;
}

static int nameserver_run() {
    int sock = nameserver_setup_listen();
    if (sock < 0) {
        return -1;
    }
    // Parse two files.
    int ret = 0;
    ret += parse_server_file(config.server_file);
    ret += parse_LSA_file(config.lsa_file);
    ret += build_routing_table();
    if (ret > 0) {
        fprintf(stderr, "Parse file error\n");
        return -1;
    }

    int retval = 0;
    node *round_robin_pt = servers;
    while (1) {
        // copy fd set for select
        fd_set ready_set = ready;
        int ready_num = select(fd_max, &ready_set, NULL, NULL, NULL);
        // error handling
        if (ready_num < 0) {
            perror("nameserver_run");
            retval = -1;
            break;
        }
        if (ready_num == 0) {
            continue;
        }
        // recv the udp packet
        char request[DNS_MSG_MAX_LEN] = {0};
        struct sockaddr_in client_sock_addr;
        socklen_t client_sock_len;
        client_sock_len = sizeof(client_sock_addr);
        int size = recvfrom(sock, request, DNS_MSG_MAX_LEN, 0, (struct sockaddr *) (&client_sock_addr),
                            &client_sock_len);
        if (size < 0) {
            perror("nameserver_run recv");
            retval = -1;
            break;
        }
        // parse the request
        char qname[512] = {0};

        int retval2 = dns_parse_request(request, size, qname);
        unsigned short dns_id = DNS_GET_ID((dns_header_t *) (request));
        client_sock_len = sizeof(client_sock_addr);

        if (retval < 0) {
            // generate response and send
            char packet[DNS_MSG_MAX_LEN + 1] = {0};
            size = dns_gen_response(qname, NULL, dns_id, 1, packet);

            size = sendto_wrap(sock, packet, size, 0, (struct sockaddr *) (&client_sock_addr), client_sock_len);
            if (size < 0) {
                perror("sendto");
            }
            break;
        }

        // Use server_ip to reply request from client.
        char *server_ip;
        char *client_ip = inet_ntoa(client_sock_addr.sin_addr); // Get this from deserialized packet.

        if (config.is_round_robin) {
            server_ip = round_robin_pt->ip;
            round_robin_pt = round_robin_pt->next;
            if (!round_robin_pt) {
                round_robin_pt = servers;
            }
        } else {
            int i;
            for (i = 0; i < total_num; i++) {
                if (strcmp(routing_table[i].client_ip, client_ip)) {
                    server_ip = routing_table[i].server_ip;
                    break;
                }
            }
        }
        // construct udp packet to send
        char packet[DNS_MSG_MAX_LEN + 1] = {0};
        size = dns_gen_response(qname, server_ip, dns_id, 0, packet);
        // keep logging
        time_t now;
        time(&now);
        log_record(config.log_file, now, client_ip, qname, server_ip);
        sendto_wrap(sock, packet, size, 0, (struct sockaddr *) (&client_sock_addr), client_sock_len);
        // end of handling iteration
    }
    close_logger();
    return 0;
}
