#ifndef __PROXY_H__
#define __PROXY_H__

#include <netinet/in.h>
#include <time.h>
#include "common.h"
#include "parse.h"
#include "bitrate.h"

#define IS_BROWSER (1)
#define IS_SERVER (0)

// connection structure
typedef struct proxy_conn {
    unsigned long T_curr;
    unsigned long t_s;
    int bitrate;
    bitrate *bitrate_list;
    browser_t browser;
    server_t server;
    char server_ip[MAX_PATH_LEN];

    struct proxy_conn *prev;
    struct proxy_conn *next;
    int server_accepted;
    state_t state;
} proxy_conn_t;

// configuration passed from cli
typedef struct proxy_config {
    char log[MAX_PATH_LEN];
    float alpha;
    int listen_port;
    struct in_addr fake_ip;
    struct in_addr dns_ip;
    int dns_port;
    struct in_addr www_ip;
    fd_set ready;
    int fd_max;
    proxy_conn_t *list_conn;
    const char *fake_ip_str;
    const char *www_ip_str;
    bitrate *default_list;
    int need_dns;
} proxy_config_t;


typedef struct ip_tpt {
    char ip[50];
    unsigned long tpt;
    struct ip_tpt *next;
} ip_tpt_t;

// init one global static congig struct
void proxy_init_config(char **argv, int www_ip);

int proxy_conn_create(int sock, proxy_conn_t *conn);

void proxy_conn_close(proxy_conn_t *conn);

int proxy_browser();

int proxy_server();

int proxy_run();

// helpers, remove and change to static after testing
void proxy_insert_conn(proxy_conn_t *conn);

void proxy_remove_conn(proxy_conn_t *conn);

void proxy_conn_init(proxy_conn_t *conn);

ip_tpt_t* ip_tpt_find(const char *ip);

void ip_tpt_add(const char *ip, unsigned long bitrate);

#endif /* !__PROXY_H__ */
