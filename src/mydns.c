#include "mydns.h"
#include "dns_record.h"
#include <sys/socket.h>
#include <stdio.h>

static int dns_id = 5, dns_sock = 0;
static struct sockaddr_in dns_addr, proxy_addr;

int init_mydns(const char *dns_ip, unsigned int dns_port, const char *my_ip) {
	memset(&dns_addr, 0, sizeof(dns_addr));
	memset(&proxy_addr, 0, sizeof(proxy_addr));
    // bind to the assigned ip address : port
    struct in_addr dns_ip_addr;
	memset(&dns_ip_addr, 0, sizeof(dns_ip_addr));
    inet_aton(dns_ip, &dns_ip_addr);
    dns_addr.sin_family = AF_INET;
    printf("-----dns port during init: %d\n", dns_port);
    dns_addr.sin_port = htons(dns_port);
    dns_addr.sin_addr = dns_ip_addr;
	// set proxy addr for binding
    struct in_addr proxy_ip_addr;
	memset(&proxy_ip_addr, 0, sizeof(proxy_ip_addr));
    inet_aton(my_ip, &proxy_ip_addr);
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(0); // any port
    proxy_addr.sin_addr = proxy_ip_addr;

    // open socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("init_mydns socket");
        return -1;
    }
    // bind to address
    int ret = bind(sock, (struct sockaddr *) (&proxy_addr), sizeof(struct sockaddr_in));
    if (ret < 0) {
        close(sock);
        perror("init_mydns bind");
        return -1;
    }
    dns_sock = sock;
    return 0;
}

int resolve(const char *node, const char *service,
            const struct addrinfo *hints, struct addrinfo **res) {
    // Resolve() should allocate a struct addrinfo, which the caller is responsible for freeing.
    *res = (struct addrinfo*) malloc(sizeof(struct addrinfo));
    memset(*res, 0, sizeof(struct addrinfo));
    (*res)->ai_addrlen = sizeof(struct sockaddr_in);
    (*res)->ai_addr = malloc(sizeof(struct sockaddr_in));
    (*res)->ai_protocol = 0;
    (*res)->ai_socktype = SOCK_STREAM;
    (*res)->ai_family = AF_INET;
    (*res)->ai_flags = AI_PASSIVE;
    (*res)->ai_next = NULL;
    // Handle request.
    char packet[DNS_MSG_MAX_LEN] = {0};
    int size = dns_generate_request(node, dns_id, packet);
    int ret = sendto(dns_sock, packet, size, 0, (struct sockaddr*) (&dns_addr), sizeof(dns_addr));
    if (ret < 0) {
        perror("resolve sendto");
        return -1;
    }
	memset(packet, 0, sizeof(packet));
    struct sockaddr_in from;
    int len = sizeof(from);
    size = recvfrom(dns_sock, packet, DNS_MSG_MAX_LEN, 0, (struct sockaddr *) (&from), (socklen_t *) (&len));
    if (size < 0) {
        perror("resolve recvfrom");
        return -1;
    }
	dns_parse_response(packet, size, res);
    return 0;
}
