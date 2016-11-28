#ifndef __PROXY_H__
#define __PROXY_H__

// #include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
// #include <arpa/inet.h>
#include "common.h"
#include "parse.h"
#include "bitrate.h"

// connection structure
typedef struct proxy_conn {
    float T_curr; // current throughput
    // handout p4: time.h -> time()
    // .tv_sec, .tv_usec
    unsigned long t_s; // t_start, t_f get when chunk finish, then update t_s
    int bitrate; // smoothed bitrate.
    bitrate *bitrate_list; // parsed from f4m
    browser_t browser;
    server_t server;
    int transmitted_char_num; // Get updated when receive chunk data from server. Used to estimate throughput.
    struct proxy_conn *prev;
    struct proxy_conn *next;
    int server_accepted;
    int state;
} proxy_conn_t;

// configuration passed from cli
typedef struct proxy_config {
    char log[MAX_PATH_LEN];
    float alpha;
    int listen_port;
    struct in_addr fake_ip; //inet_aton("63.161.169.137", &myaddr.sin_addr.s_addr);
    struct in_addr dns_ip;
    int dns_port;//htons
    struct in_addr www_ip;
    fd_set ready;
    int fd_max;
    proxy_conn_t *list_conn;
} proxy_config_t;


/*
struct sockaddr_in myaddr;
int s;

myaddr.sin_family = AF_INET;
myaddr.sin_port = htons(3490);
inet_aton("63.161.169.137", &myaddr.sin_addr.s_addr);

s = socket(PF_INET, SOCK_STREAM, 0);
bind(s, (struct sockaddr*)myaddr, sizeof(myaddr));
*/

// init one global static congig struct
void proxy_init_config(char **argv, int www_ip);

int proxy_conn_create(int sock, proxy_conn_t *conn);

void proxy_conn_close();

int proxy_browser();

int proxy_server();

int proxy_run();

// helpers, remove and change to static after testing
void proxy_insert_conn(proxy_conn_t *conn);

void proxy_remove_conn(proxy_conn_t *conn);

void proxy_conn_init(proxy_conn_t *conn);

#endif /* !__PROXY_H__ */