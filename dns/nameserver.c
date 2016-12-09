//
// Created by yayunh on 11/22/16.
//
#include <time.h>
#include "nameserver.h"
#include "../src/dns_record.h"

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
    //inet_pton(AF_INET, config.ip, &(nameserver_addr.sin_addr));
	struct in_addr ns_inaddr;
    inet_aton(config.ip, &ns_inaddr);
    nameserver_addr.sin_addr = ns_inaddr;
    nameserver_addr.sin_port = htons(config.listen_port);
    // bind to address
    if (bind(sock, (struct sockaddr *) (&nameserver_addr), sizeof(struct sockaddr_in)) < 0) {
        perror("nameserver_setup_listen bind");
        return -1;
    }
    // listen to port
    //if (listen(sock, PROXY_MAX_LISTEN) < 0) {
    //    perror("nameserver_setup_listen listen");
    //    return -1;
    //}
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

    start_logger("mylog.txt");
    log_init(config.log_file);
    // run nameserver
    return nameserver_run();
}

int sendto_wrap(int sock, char* buf, size_t size, int flag, struct sockaddr *addr, socklen_t addr_len) {
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
	puts("1");
    int sock = nameserver_setup_listen();
	puts("2");
    if (sock < 0) {
        return -1;
    }
    // Parse two files.
    int ret = 0;
	puts("3");
    ret += parse_server_file(config.server_file);
	puts("4");
    ret += parse_LSA_file(config.lsa_file);
	puts("5");
    ret += build_routing_table();
	puts("6");
    if (ret > 0) {
        fprintf(stderr, "Parse file error\n");
        return -1;
    }
	puts("7");

    int retval = 0;
    while (1) {
		puts("8");
        // copy fd set for select
        fd_set ready_set = ready;
		puts("before select");
        int ready_num = select(fd_max, &ready_set, NULL, NULL, NULL);
		puts("after select");
        // error handling
        if (ready_num < 0) {
            perror("nameserver_run");
            retval = -1;
            break;
        }
        if (ready_num == 0) {
            puts("should not happen since timeout not set");
            continue;
        }
        // recv the udp packet
        char request[DNS_MSG_MAX_LEN] = {0};
		printf("request pt: %p\n", request);
		struct sockaddr_in client_sock_addr;
		socklen_t client_sock_len;
		client_sock_len = sizeof(client_sock_addr);
        int size = recvfrom(sock, request, DNS_MSG_MAX_LEN, 0, (struct sockaddr *) (&client_sock_addr), &client_sock_len);
        if (size < 0) {
            perror("nameserver_run recv");
            retval = -1;
            break;
        }
        // parse the request
        char qname[512] = {0};
        //retval = dns_parse_request(request, size, qname);
        int kk = 0;
        for (; kk < size; kk++) {
          logout("%c", request[kk]);
        }
        close_logger();
        printf("size: %d\n", size);
        int retval2 = dns_parse_request(request, size, qname);
		printf("qname: %s\n", qname);
		printf("retval: %d\n", retval2);
		puts("9");
        unsigned short dns_id = DNS_GET_ID((dns_header_t*)(request)); // TODO: check here, might not be ok
		printf("dns_id: %d\n", dns_id);
		client_sock_len = sizeof(client_sock_addr);

		printf("retval: %d\n", retval);
        if (retval < 0) {
			printf("parse request error\n");
			// generate response and send
			char packet[DNS_MSG_MAX_LEN + 1] = {0};
			size = dns_gen_response(qname, NULL, dns_id, 1, packet);
			printf("generate packet result: %d\n", size);
			// TODO
			printf("client sockaddr: %u\n", client_sock_addr);
			printf("client ip: %s\n", inet_ntoa(client_sock_addr.sin_addr));
			size = sendto_wrap(sock, packet, size, 0, (struct sockaddr *) (&client_sock_addr), client_sock_len);
			printf("sendto result: %d\n", size);
			if (size < 0) {
				perror("sendto");
			}
            break;
        }

        // TODO(hanlins): use server_ip to reply request from client.
        //************************************************************
        char *server_ip;
        char *client_ip; // Get this from deserialized packet.

        node *round_robin_pt = servers;
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
        // construct udp packet to send
        char packet[DNS_MSG_MAX_LEN + 1] = {0};
        size = dns_gen_response(qname, server_ip, dns_id, 0, packet);
		printf("response size: %d\n", size);;
        // keep logging
        time_t now;
        time(&now);
        log_record(config.log_file, now, client_ip, qname, server_ip);
        sendto_wrap(sock, packet, size, 0, (struct sockaddr *) (&client_sock_addr), client_sock_len);
		puts("10");
        // end of handling iteration
    }
    close_logger();
    return 0;
}
