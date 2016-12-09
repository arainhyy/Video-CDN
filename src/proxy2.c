#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include "proxy2.h"
#include "helper.h"
#include "log.h"
#include "parse.h"
#include "mydns.h"

#define PROXY_MAX_LISTEN (5)
#define PROXY_FD_BROWSER (1 << 0)
#define PROXY_FD_SERVER  (1 << 1)

/** Static global configuration */
//static proxy_config_t config;
proxy_config_t config; // make it available for testing
ip_tpt_t *ip_tpt_list = NULL;

unsigned long get_mill_time();
/**
 * @brief Setup proxy til listen stage.
 *
 * Setup proxy til listen stage.
 *
 * @return Negative on error, zero on success.
 */
static int proxy_setup_listen();

/**
 * @brief Proxy handle connection.
 *
 * Handle proxy connection.
 *
 * @param conn Pointer to connection.
 * @param fd_flag Flag indicating browser / server fd set.
 * @return Negative on error, zero on success.
 */
static int proxy_handle_conn(proxy_conn_t *conn, int fd_flag);

/**
 * @brief Handle browser's request.
 *
 * Handle browser's request.
 *
 * @param conn Pointer to connection.
 * @return Negative on error, zero on success.
 */
static int handler_browser(proxy_conn_t *conn);

/**
 * @brief Handle server's response.
 *
 * Handle server's response.
 *
 * @param conn Pointer to connection.
 * @return Negative on error, zero on success.
 */
static int handler_server(proxy_conn_t *conn);

static int browser_html(proxy_conn_t *conn);

static int browser_f4m(proxy_conn_t *conn);

static int browser_chunk(proxy_conn_t *conn);

static int handle_resp_html(proxy_conn_t *conn);

static int handle_resp_f4m(proxy_conn_t *conn);

static int handle_resp_chunk(proxy_conn_t *conn);

static int handle_resp_f4m_nolist(proxy_conn_t *conn);

int handle_server(proxy_conn_t *conn);

void estimate_throughput(proxy_conn_t *conn, unsigned long chunk_size);

static void clear_parsed_request(proxy_conn_t *conn, int is_browser);

/**
 * @brief Initialize command line configuration.
 *
 * Initialize command line configuration.
 *
 * @param argv Argument list.
 * @param www_ip Whether www_ip is entered.
 * @return Void.
 */
void proxy_init_config(char **argv, int www_ip) {
    start_logger("my_log.txt");
    strncpy(config.log, argv[1], MAX_PATH_LEN);
    config.alpha = (float) atof(argv[2]);
    config.listen_port = atoi(argv[3]);
    inet_aton(argv[4], &config.fake_ip);
    config.fake_ip_str = argv[4];
    inet_aton(argv[5], &config.dns_ip);
    config.dns_port = atoi(argv[6]);
    config.www_ip.s_addr = (long) -1;
    if (www_ip == 1) {
		puts("has www_ip");
        inet_aton(argv[7], &config.www_ip);
        config.www_ip_str = argv[7];
        config.need_dns = 0;
    } else {
        // otherwise, not use hard-coded ip addr
        init_mydns(argv[5], argv[6], argv[4]);
        config.need_dns = 1;
    }
    config.list_conn = NULL;
    config.default_list = NULL;
    // init the dns setting on client
//	log_init(config.log);
}

int proxy_conn_create(int sock, proxy_conn_t *conn) {
    puts("enter conn create");
    proxy_conn_init(conn);
    // init browser struct
    struct sockaddr_in browser_addr;
    socklen_t addr_len = sizeof(browser_addr);
    // accept from browser
    int browser_sock = accept(sock, (struct sockaddr_in *) (&browser_addr),
                              &addr_len);
    if (browser_sock < 0) {
        perror("proxy_conn_create");
        return -1;
    }
    // set browser's fd
    conn->browser.fd = browser_sock;
	FD_SET(browser_sock, &config.ready);
	printf("fdset: %d\n", browser_sock);
    if (browser_sock >= config.fd_max) {
        config.fd_max = browser_sock + 1;
    }
    // save ip
    char *ip = inet_ntoa(browser_addr.sin_addr);
    strcpy(conn->browser.ip, ip);
    printf("-------browser.ip--%s\n", conn->browser.ip);
    if (ip_tpt_find(conn->browser.ip) > 0) {
      conn->T_curr = ip_tpt_find(conn->browser.ip)->tpt;
    }
    printf("tcurr init:%lu\n", conn->T_curr);
    // insert to list
    proxy_insert_conn(conn);
    return 0;
}

void proxy_conn_close(proxy_conn_t *conn) {
    printf("remove conn %d\n", conn->browser.fd);
    // close sockets, browser, server
    if (conn->browser.fd != 0) {
        close(conn->browser.fd);
        FD_CLR(conn->browser.fd, &config.ready);
		printf("fdclr: %d\n", conn->browser.fd);
        conn->browser.fd = 0;
    }
    if (conn->server.fd != 0) {
        close(conn->server.fd);
        FD_CLR(conn->server.fd, &config.ready);
		printf("fdclr: %d\n", conn->server.fd);
        conn->server.fd = 0;
    }
    // remove from conn list
    proxy_remove_conn(conn);
}

int proxy_run() {
    // bind to ip
    // INADDR_ANY
    // This allowed your program to work without knowing the IP address of
    // the machine it was running on, or, in the case of a machine with
    // multiple network interfaces, it allowed your server to receive packets
    // destined to any of the interfaces. In reality, the semantics of INADDR_ANY
    // are more complex and involved.
    //
    // search result from:
    // https://www.cs.cmu.edu/~srini/15-441/F01.full/www/assignments/P2/htmlsim_split/node18.html
    int sock = proxy_setup_listen();

    if (sock < 0) {
        return sock;
    }

    // while loop for req handling
    int retval = 0;
    while (1) {
        // copy fd set for select
        fd_set ready = config.ready;
        int ready_num = select(config.fd_max, &ready, NULL, NULL, NULL);
        // error handling
        if (ready_num < 0) {
            perror("proxy_run");
            retval = -1;
            break;
        }
        if (ready_num == 0) {
            puts("should not happen since timeout not set");
            continue;
        }
        // now ready_num must larger than 0
        // first handle new connection
        if (FD_ISSET(sock, &ready)) {
            logout("create new connection sock: %d\n", sock);
            proxy_conn_t *new_conn = malloc(sizeof(proxy_conn_t));
            if (new_conn) {
                proxy_conn_create(sock, new_conn);
            } else {
                perror("proxy_conn_create malloc");
            }
            ready_num--;
        }
        proxy_conn_t *curr = config.list_conn; // iterator
        // iterate the connection list to handle requests
        while ((curr != NULL) && (ready_num > 0)) {
            int fd_flag = 0;
            if (FD_ISSET(curr->browser.fd, &ready)) {
                fd_flag |= PROXY_FD_BROWSER;
                ready_num--;
            }
            if (FD_ISSET(curr->server.fd, &ready)) {
                fd_flag |= PROXY_FD_SERVER;
                ready_num--;
            }
            if (!fd_flag) {
                curr = curr->next;
                continue;
            }
            int ret = proxy_handle_conn(curr, fd_flag);
            if (ret < 0) {
                // error handling, remove from connection list?
                // also free resource
                proxy_conn_close(curr);
            }
            // update curr pointer
            curr = curr->next;
        }
    }
    return retval;
}

static int proxy_setup_listen() {
    FD_ZERO(&config.ready);
    // create socket ipv4
//    int sock = socket(AF_INET, SOCK_STREAM, PF_INET);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("proxy_setup_listen socket");
        return -1;
    }
    // set sock for incoming connections
    FD_SET(sock, &config.ready);
	printf("listen to fd %d\n", sock);
    config.fd_max = sock + 1;
    // initialize address
    struct sockaddr_in proxy_addr;
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY); // specified in handout
    proxy_addr.sin_port = htons(config.listen_port);
    // bind to address
    if (bind(sock, (struct sockaddr *) (&proxy_addr), sizeof(struct sockaddr_in)) < 0) {
        perror("proxy_setup_listen bind");
        return -1;
    }
    // listen to port
    if (listen(sock, PROXY_MAX_LISTEN) < 0) {
        perror("proxy_setup_listen listen");
        return -1;
    }
    return sock;
}

static int proxy_connect_server(proxy_conn_t *conn) {
	puts("connect server");
    // initialize address
    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(struct sockaddr_in));
    proxy_addr.sin_family = AF_INET;
//    proxy_addr.sin_addr.s_addr = config.fake_ip.s_addr; // specified in handout
    proxy_addr.sin_addr.s_addr = inet_addr(config.fake_ip_str);
    proxy_addr.sin_port = htons(0); // ephemeral

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (config.www_ip.s_addr != -1) {
//        server_addr.sin_addr.s_addr = config.www_ip.s_addr;
        server_addr.sin_addr.s_addr = inet_addr(config.www_ip_str);
    }
    // create socket ipv4
    //int sock = socket(AF_INET, SOCK_STREAM, PF_INET);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("proxy_connect_server socket");
        return -1;
    }
    int ret = bind(sock, (struct sockaddr *) (&proxy_addr), sizeof(struct sockaddr));
    if (ret < 0) {
		puts("bind error!");
        perror("proxy_connect_server bind");
        close(sock);
        return -1;
    }
	// try to resolve now
	puts("try to resolve now");
    if (config.need_dns) {
        // use dns function to get the ip
        struct addrinfo *addr_result;
        ret = resolve("video.cs.cmu.edu", "8080", NULL, &addr_result);
        if (ret >= 0) {
            server_addr.sin_addr = ((struct sockaddr_in*)addr_result->ai_addr)->sin_addr;
            free(addr_result);
            ret = connect(sock, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr));
        }
        // otherwise just return ret < 0
    } else {
        ret = connect(sock, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr));
    }
    if (ret < 0) {
		puts("connection error!");
        perror("proxy_connect_server connect");
        close(sock);
        return -1;
    }
    // init server in conn
    FD_SET(sock, &config.ready);
	printf("fdset: %d\n", sock);
    if (sock >= config.fd_max) {
        config.fd_max = sock + 1;
    }
    conn->server.fd = sock;
    conn->server.request = NULL;
    conn->server.type = -1;
    conn->server.offset = 0;
    conn->server.to_send_length = 0;
    conn->server.f4m_request[0] = '\0';
    conn->server.response_body = NULL;
    conn->server.response[0] = '\0';
    return sock;
}

static int proxy_handle_conn(proxy_conn_t *conn, int fd_flag) {
    if (fd_flag & PROXY_FD_BROWSER) {
        // handle browser's request
        int ret = handler_browser(conn); // handle result
        if (ret < 0) {
            return -1;
        }
    }
    if (fd_flag & PROXY_FD_SERVER) {
        // handle server's response
        int ret = handler_server(conn); // handle result
        if (ret < 0) {
            return -1;
        }
    }
    return 0;
}

static int handler_browser(proxy_conn_t *conn) {
    // read from socket
    printf("*********************** handle browser: %d server:%d\n", conn->browser.fd, conn->server.fd);
    int old_offset = conn->browser.offset;
    int recvlen = recv(conn->browser.fd, conn->browser.buf + conn->browser.offset,
                       MAX_REQ_SIZE - conn->browser.offset, MSG_DONTWAIT);
    if (recvlen <= 0) {
        perror("handler_browser recv");
        return -1;
    }
    conn->browser.offset += recvlen;
    // parse request
    conn->browser.request = parse(conn->browser.buf, recvlen);
    if (conn->browser.request->status < 0) {
        printf("Incomplete request---------------\n");
        return 0;
    }
    conn->browser.offset -= conn->browser.request->position;
    if (conn->browser.offset > 0) {
        memmove(conn->browser.buf, conn->browser.buf + conn->browser.request->position,
                conn->browser.offset);
    }
    // check request type
    conn->browser.type = check_type(conn->browser.request);
    // connect to server
    if (proxy_connect_server(conn) < 0) {
		puts("conn server fail");
        return -1;
    }
	printf("req type: %d\n", conn->browser.type);
    // check whether it is chunk request, set flag
    int ret = -1;
    if (conn->browser.type == REQ_CHUNK) {
        if (conn->bitrate_list == NULL) {
          conn->bitrate_list = dup_bitrate_list(config.default_list);
        }
        printf("tcurr %s before select: %lu\n", conn->browser.ip, conn->T_curr);
        conn->bitrate = select_bitrate(conn->bitrate_list, conn->T_curr);
        conn->t_s = get_mill_time();
        // forward request directly
        char buf[MAX_REQ_SIZE] = {0};
        int len = construct_http_req(buf, conn->browser.request);
        get_video_name(conn->browser.request->http_uri, conn->server.chunk_name);
        // replace bitrate
        replace_uri_bitrate(buf, conn->bitrate);
        ret = send_data(conn->server.fd, buf, len);
    } else if (conn->browser.type == REQ_F4M) {
        // save f4m
        memcpy(conn->server.f4m_request, conn->browser.buf + old_offset, recvlen);
		puts("-----------------REQ_F4M_MSG---------------");
		int i = 0;
		for (; i < recvlen; i++) {
			printf("%c", conn->server.f4m_request[i]);
		}
		puts("");
		puts("-----------------REQ_F4M_MSG---------------");
        ret = proxy_req_forward(conn);
    } else {
        ret = proxy_req_forward(conn);
    }
    // update state
    switch (conn->browser.type) {
        case REQ_HTML:
            puts("----------------REQ_HTML-----------------");
            conn->state = HTML;
            break;
        case REQ_F4M:
            puts("----------------REQ_F4M-----------------");
            conn->state = F4M;
            break;
        case REQ_CHUNK:
            puts("----------------REQ_CHUNK-----------------");
            conn->state = CHUNK;
            break;
        default:
            conn->state = HTML;
            break;
    }
    clear_parsed_request(conn, IS_BROWSER);
    puts("end handle browser");
    return ret;
}

static int handler_server(proxy_conn_t *conn) {
    // read from socket
    printf("*********************** handle server: %d browser: %d\n", conn->server.fd, conn->browser.fd);
    int recvlen = recv(conn->server.fd, conn->server.buf + conn->server.offset,
                       MAX_REQ_SIZE - conn->server.offset, MSG_DONTWAIT);
    if (recvlen <= 0) {
        perror("handler_server recv");
        return -1;
    }
    conn->server.offset += recvlen;
    int ret = 0;
    while (conn->server.offset > 0 && (conn->server.request == NULL || conn->server.request->status != NEEDMORE)
        && ret != -1) {
        // check if it has sent all content body of last request to client.
        if (conn->server.to_send_length > 0) {
            puts("enter read body");
            int to_send = conn->server.to_send_length > conn->server.offset ?
                          conn->server.offset : conn->server.to_send_length;
            int old_len_body = conn->server.request->content_length - conn->server.to_send_length;
            memmove(conn->server.response_body + old_len_body, conn->server.buf, to_send);
            conn->server.response_body[old_len_body + to_send] = '\0';
            conn->server.to_send_length -= to_send;
            // Update buffer and offset.
            conn->server.offset -= to_send;
            if (conn->server.offset > 0) {
                memmove(conn->server.buf, conn->server.buf + to_send, conn->server.offset);
            }
            puts("leave read body");
        }
        if (conn->server.request != NULL && conn->server.request->status != NEEDMORE && conn->server.to_send_length == 0) {
            puts("enter handle request");

            switch (conn->state) {
                case HTML:
                    puts("----------------RESP_HTML-----------------");
                    ret = handle_resp_html(conn);
                    break;
                case F4M_NOLIST:
                    puts("----------------RESP_NOLIST-----------------");
                    ret = handle_resp_f4m_nolist(conn);
                    break;
                case F4M:
                    puts("----------------RESP_F4M-----------------");
                    ret = handle_resp_f4m(conn);
                    break;
                case CHUNK:
                    puts("----------------RESP_CHUNK-----------------");
                    ret = handle_resp_chunk(conn);
                    break;
                default:ret = -1;
            }
            if (conn->server.response_body != NULL) {
                free(conn->server.response_body);
                conn->server.response_body = NULL;
            }
            free(conn->server.request);
            conn->server.request = NULL;
            puts("leave handle request");
        }

        if (conn->server.offset <= 0) break;
        // parse request.
        conn->server.request = parse_reponse(conn->server.buf, recvlen);
        if (conn->server.request->status < 0) {
            printf("Incomplete request---------------%d\n", conn->server.offset);
            return ret;
        }
        conn->server.to_send_length = conn->server.request->content_length;
        conn->server.response_body = malloc(sizeof(char) * (conn->server.to_send_length + 1));
        conn->server.response_body[0] = '\0';

        conn->server.offset -= conn->server.request->position;
        // Store server response before clear server receiving buffer.

        memcpy(conn->server.response, conn->server.buf, conn->server.request->position);
        conn->server.response[conn->server.request->position] = '\0';
        if (conn->server.offset > 0) {
            memmove(conn->server.buf, conn->server.buf + conn->server.request->position,
                    conn->server.offset);
        }
    }
    puts("Finish handle server");
    return ret;
}

// should init the conn before insert
void proxy_insert_conn(proxy_conn_t *conn) {
    if (!conn) {
        return;
    }
    proxy_conn_t *temp = config.list_conn;
    config.list_conn = conn;
    conn->next = temp;
    if (temp) {
        temp->prev = conn;
    }
}

void proxy_remove_conn(proxy_conn_t *conn) {
    if (!conn) {
        return;
    }
    proxy_conn_t *prev = conn->prev, *next = conn->next;
    if (prev) {
        prev->next = next;
    }
    if (next) {
        next->prev = prev;
    }
    // check whether head
    if (!prev) {
        config.list_conn = next;
    }
}

void proxy_conn_init(proxy_conn_t *conn) {
    conn->T_curr = 0;
    conn->t_s = 0;
    conn->bitrate = 0;
    conn->bitrate_list = NULL;
    memset(&conn->browser, 0, sizeof(conn->browser));
    memset(&conn->server, 0, sizeof(conn->server));
    conn->prev = NULL;
    conn->next = NULL;
    conn->server_accepted = 0;
    conn->server_accepted = 0;
    conn->state = 0;
}

unsigned long get_mill_time();

/**
 * Estimate smoothed throughput by time and chunk size.
 *
 * @param conn
 * @param chunk_size
 */
void estimate_throughput(proxy_conn_t *conn, unsigned long chunk_size) {
  unsigned long t_finish = get_mill_time();
  // Exchange T to Kbps.
  unsigned long duration = t_finish - conn->t_s;
  float diff = duration / 1000.0;
  unsigned long T = ((chunk_size / diff) / 1000) * 8;
  unsigned long Tcurrent;
  ip_tpt_t *tpt = ip_tpt_find(conn->browser.ip);
  if (!tpt) {
    ip_tpt_add(conn->browser.ip, 0);
  }
  Tcurrent = ip_tpt_find(conn->browser.ip)->tpt;
  Tcurrent = config.alpha * T + (1.0 - config.alpha) * Tcurrent;
  conn->T_curr = (int) Tcurrent;
  // save to history
  ip_tpt_add(conn->browser.ip, Tcurrent);
  replace_uri_bitrate(conn->server.chunk_name, conn->bitrate);
  //log_record(config.log, t_finish / 1000000, duration / 1000.0, T, Tcurrent, conn->bitrate,
  log_record(config.log, t_finish / 1000, diff, T, Tcurrent, conn->bitrate,
             config.www_ip_str, conn->server.chunk_name);
}

/* Get timestamp in milliseconds. */
unsigned long get_mill_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long mill = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
    return mill;
}

int proxy_req_forward(proxy_conn_t *conn) {
    // forward request directly
    char buf[MAX_REQ_SIZE] = {0};
    int len = construct_http_req(buf, conn->browser.request);
  char tmp_req[MAX_REQ_SIZE];
  memmove(tmp_req, buf, len);
  tmp_req[len] = '\0';
  printf("send non chunk request: %s\n", tmp_req);
    return send_data(conn->server.fd, buf, len);
}

static int handle_resp_html(proxy_conn_t *conn) {
    // forward response directly
    int ret_1 = 0, ret_2 = 0;

    ret_1 = send_data(conn->browser.fd, conn->server.response, strlen(conn->server.response));
    ret_2 += send_data(conn->browser.fd, conn->server.response_body, conn->server.request->content_length);
    if (ret_1 < 0 || ret_2 < 0) {
        return -1;
    }
    return ret_1 + ret_2;
}

static int handle_resp_f4m(proxy_conn_t *conn) {
    // 1. parse xml and get list of bitrates
    conn->bitrate_list = parse_xml_to_list(conn->server.response_body);
    if (config.default_list == NULL) {
      config.default_list = dup_bitrate_list(conn->bitrate_list);
    }
    // 2. request for nolist version
    replace_f4m_to_nolist(conn->server.f4m_request);
    int ret = send_data(conn->server.fd, conn->server.f4m_request, strlen(conn->server.f4m_request));
    // 3. set state
    conn->state = F4M_NOLIST;
    return ret;
}

static int handle_resp_f4m_nolist(proxy_conn_t *conn) {
    // forward response directly
    int ret_1 = 0, ret_2 = 0;
    ret_1 = send_data(conn->browser.fd, conn->server.response, strlen(conn->server.response));
    ret_2 += send_data(conn->browser.fd, conn->server.response_body, conn->server.request->content_length);
    if (ret_1 < 0 || ret_2 < 0) {
        return -1;
    }
    return ret_1 + ret_2;
}

static int handle_resp_chunk(proxy_conn_t *conn) {
    estimate_throughput(conn, conn->server.request->content_length);
//    conn->bitrate = select_bitrate(conn->bitrate_list, conn->T_curr);
    int ret_1 = 0, ret_2 = 0;
    ret_1 = send_data(conn->browser.fd, conn->server.response, strlen(conn->server.response));
    ret_2 = send_data(conn->browser.fd, conn->server.response_body, conn->server.request->content_length);
    if (ret_1 < 0 || ret_2 < 0) {
        return -1;
    }
    return ret_1 + ret_2;
}

/**
 * Clear parsed request and its parsed headers.
 *
 * @param conn
 * @param is_browser indicates to clear browser parsed request or server parsed response.
 */
void clear_parsed_request(proxy_conn_t *conn, int is_browser) {
    struct Request_header *tmp;
    if (is_browser) {
        tmp = conn->browser.request->headers;
    } else {
        tmp = conn->server.request->headers;
    }
    struct Request_header *pre;
    while (tmp != NULL) {
        pre = tmp;
        tmp = tmp->next;
        free(pre);
    }
    if (is_browser) {
        free(conn->browser.request);
        conn->browser.request = NULL;
    } else {
        free(conn->server.request);
        conn->server.request = NULL;
    }
}

ip_tpt_t* ip_tpt_find(const char *ip) {
    ip_tpt_t *curr = ip_tpt_list;
    while (curr != NULL) {
        if (strstr(curr->ip, ip)) {
            break;
        }
        curr = curr->next;
    }
    return curr;
}

void ip_tpt_add(const char *ip, unsigned long tpt) {
    printf("update tpt: %s-%d\n", ip, tpt);
    ip_tpt_t *target = ip_tpt_find(ip);
    if (!target) {
        printf("not found\n");
        target = malloc(sizeof(ip_tpt_t));
        if (!target) {
            return;
        }
        target->next = ip_tpt_list;
        ip_tpt_list = target;
        strcpy(target->ip, ip);
    }
    target->tpt = tpt;
}
