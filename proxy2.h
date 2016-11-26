#ifndef __PROXY_H__
#define __PROXY_H__

// #include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
// #include <arpa/inet.h>
#include "common.h"

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
} proxy_config_t;

// connection structure
typedef struct proxy_conn {
    // browser_info
    // server_info
    server_conn_t * s_conn;
    float T_curr; // current throughput
    // handout p4: time.h -> time()
    // .tv_sec, .tv_usec
    time_t t_s; // t_start, t_f get when chunk fininsh, then update t_s
    int bitrate; // parsed from f4m
    // vqueue, link list part, Q_...
    // ?? use pointer or static?
    // browser_info
    // server_info
} proxy_conn_t;

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
void proxy_conn_create();
void proxy_conn_close();
int proxy_browser();
int proxy_server();
int proxy_run();

#endif /* !__PROXY_H__ */




// scratch
// @file browser.h
typedef struct browser {
	// browser request information
	int fd; // socket fd
	char buf[MAX_REQ_SIZE]; // load request in this fielf for parsing?
	// following are fields parsed after parsing
	// req type..., int / enum
	// server ip
	header_t header; // parsed http header
	// type
	enum ... type;
} browser_t;



int browser_parse_request(browser_t *req); // parse the request header
int browser_modify_request(browser_t *req); // modify the req before forward


// @file server.h
typedef struct server {
	// server response information
	int fd;
	char buf[MAX_REQ_SIZE];
	header_t header;
	// pointer to response body
	char *response;
	// size of response
	int size;
	// type
	enum ... type;
} server_t;

int server_parse_response(server_t *resp); // maybe can use same as browser's parser for parsing resource type
// error handling when malloc failed?


// @file helper.h

// encapsulation of sending data and error handling
// return 0 on succ, neg on error
// used to send response to browser and request to server
// maybe should move this function to proxy module?? since currently
// no other module use this...
int send_data(int fd, char *buf, int len);

int send_data(int fd, char *buf, int len) {
	int total_sent = 0;
	while (total_sent < len) {
		int ret = send(fd, buf + total_sent, (len - total_sent), 0);
		if (ret < 0) {
			perror("send_data");
			return -1;
		}
		// update total_sent
		total_sent += ret;
	}
	return 0;
}

int get_header(char *buf); // get http header, shared by browser and server??

// when handle chunk request, update throughput, and log it
int update_throughput(proxy_conn_t *conn);
int update_throughput(proxy_conn_t *conn) {
	time_t t_s = conn->t_s;
	time_t t_f;
	time(&t_f);
	// calculate difference in seconds
	long time_diff = t_f.tv_sec - t_s.tv_sec;
	log(...);

}

void log(...);
