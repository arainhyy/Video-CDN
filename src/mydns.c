#include "mydns.h"


int init_mydns(const char *dns_ip, unsigned int dns_port) {
    // bind to the assigned ip address : port
    struct in_addr dns_ip_addr;
    inet_aton(dns_ip, &dns_ip_addr);
    struct sockaddr_in dns_addr;
    dns_addr.sin_family = AF_INET;
    dns_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dns_addr.sin_port = htons(dns_port);
    dns_addr.sin_addr = dns_ip_addr;
    // open socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("init_mydns socket");
        return sock;
    }
    // bind to address
    int ret = bind(sock, (struct sockaddr *) (&dns_addr), sizeof(struct sockaddr_in));
    if (ret < 0) {
        close(sock);
        perror("init_mydns bind");
        return ret;
    }
    return sock;
}

int resolve(const char *node, const char *service,
            const struct addrinfo *hints, struct addrinfo **res) {
    return 0;
}
