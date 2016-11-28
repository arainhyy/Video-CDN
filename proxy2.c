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

#define PROXY_MAX_LISTEN (5)
#define PROXY_FD_BROWSER (1 << 0)
#define PROXY_FD_SERVER  (1 << 1)

/** Static global configuration */
//static proxy_config_t config;
proxy_config_t config; // make it available for testing

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

int handle_server(proxy_conn_t *conn);
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
    strncpy(config.log, argv[1], MAX_PATH_LEN);
    config.alpha = (float) atof(argv[2]);
    config.listen_port = atoi(argv[3]);
    inet_aton(argv[4], &config.fake_ip);
    inet_aton(argv[5], &config.dns_ip);
    config.dns_port = atoi(argv[6]);
    config.www_ip.s_addr = (long) -1;
    if (www_ip == 1) {
        inet_aton(argv[7], &config.www_ip);
    }
    config.list_conn = NULL;
}

int proxy_conn_create(int sock, proxy_conn_t *conn) {
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
    // insert to list
    proxy_insert_conn(conn);
    return 0;
}

void proxy_conn_close(proxy_conn_t *conn) {
    // close sockets, browser, server
    if (conn->browser.fd != 0) {
        close(conn->browser.fd);
        FD_CLR(conn->browser.fd, &config.ready);
        conn->browser.fd = 0;
    }
    if (conn->server.fd != 0) {
        close(conn->server.fd);
        FD_CLR(conn->server.fd, &config.ready);
        conn->server.fd = 0;
    }
    // remove from conn list
    proxy_remove_conn(conn);
}

int proxy_browser() {
    // parse request from browser, if error return neg
    // browser_parse_request(proxy_conn_t -> browser_t)

    // modify request as specified, IMPORTANT@p5
    // modify_browser_request(proxy_conn_t -> browser_t)
    // also need to update connection's t_s here IF REQUEST IS A CUNK!!

    // send modified request to server
    // send_request(...->server's fd, request buffer)
    // cleanup request buffer?

}

int proxy_server() {
    // forward the data sent by server

    // send the data to client's fd

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
            proxy_conn_t *new_conn = malloc(sizeof(proxy_conn_t));
            if (new_conn) {
                proxy_conn_create(sock, new_conn);
            } else {
                perror("proxy_conn_create malloc");
            }
            ready_num--;
        }
        proxy_conn_t * curr = config.list_conn; // iterator
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
    int sock = socket(AF_INET, SOCK_STREAM, PF_INET);
    if (sock < 0) {
        perror("proxy_setup_listen socket");
        return -1;
    }
    // set sock for incoming connections
    FD_SET(sock, &config.ready);
    config.fd_max = sock;
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
    // initialize address
    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(struct sockaddr_in));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = config.fake_ip.s_addr; // specified in handout
    proxy_addr.sin_port = htons(0); // ephemeral

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (config.www_ip.s_addr != -1) {
        server_addr.sin_addr.s_addr = config.www_ip.s_addr;
    }
    // create socket ipv4
    int sock = socket(AF_INET, SOCK_STREAM, PF_INET);
    if (sock < 0) {
        perror("proxy_connect_server socket");
        return -1;
    }
    int ret = bind(sock, (struct sockaddr*) (&proxy_addr), sizeof(struct sockaddr));
    if (ret < 0) {
        perror("proxy_connect_server bind");
        close(sock);
        return -1;
    }
    ret = connect(sock, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr));
    if (ret < 0) {
        perror("proxy_connect_server connect");
        close(sock);
        return -1;
    }
    // init server in conn
    FD_SET(sock, &config.ready);
    if (sock > config.fd_max) {
        config.fd_max = sock;
    }
    conn->server.fd = sock;
    conn->server.request = NULL;
    conn->server.type = -1;
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
    int recvlen = recv(conn->browser.fd, conn->browser.buf + conn->browser.offset,
                     MAX_REQ_SIZE - conn->browser.offset, MSG_DONTWAIT);
    if (recvlen < 0) {
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
        return -1;
    }
    // check whether it is chunk request, set flag
    if (conn->browser.is_chunk) {
        // @TODO, change uri to best bitrate
    }
    switch (conn->browser.type) {
        case REQ_HTML:
            conn->state = HTML;
            break;
        case REQ_F4M:
            conn->state = F4M;
            break;
        case REQ_CHUNK:
            conn->state = CHUNK;
            break;
        default:
            return -1;
    }
    return proxy_req_forward(conn);
//    int ret = -1;
//    switch (conn->browser.type) {
//        case REQ_HTML:
//            // handle html
////            ret = browser_html(conn);
//            break;
//        case REQ_F4M:
//            // handle f4m
////            ret = browser_f4m(conn);
//            break;
//        case REQ_CHUNK:
//            // handle chunk
//            // modify to adapt to bitrate
////            ret = browser_chunk(conn);
//            break;
//        default:
//            ret = -1;
//    }
//    return ret;
}

static int handler_server(proxy_conn_t *conn) {
    // read from socket
    int recvlen = recv(conn->server.fd, conn->server.buf + conn->server.offset,
                       MAX_REQ_SIZE - conn->server.offset, MSG_DONTWAIT);
    if (recvlen < 0) {
        perror("handler_server recv");
        return -1;
    }
    conn->server.offset += recvlen;
    // parse request
    conn->server.request = parse(conn->server.buf, recvlen);
    if (conn->server.request->status < 0) {
        printf("Incomplete request---------------\n");
        return 0;
    }
    conn->server.offset -= conn->server.request->position;
    if (conn->server.offset > 0) {
        memmove(conn->server.buf, conn->server.buf + conn->server.request->position,
                  conn->server.offset);
    }
    // check request type
    // TODO: differentiate request and response types
    conn->server.type = check_type(conn->server.request);
    int ret = -1;
    switch (conn->state) {
        case HTML:
            break;
        case F4M_NOLIST:
            // handle html
            break;
        case F4M:
            // handle f4m
            ret = handle_resp_f4m(conn);
            break;
        case CHUNK:
            // handle chunk
            // modify to adapt to bitrate
            ret = handle_resp_chunk(conn);
            break;
        default:
            ret = -1;
    }
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
    conn->prev = NULL;
    conn->next = NULL;
    conn->server_accepted = 0;
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
    unsigned long T = (double)conn->transmitted_char_num * 1000.0 * 8.0 / (t_finish - conn->t_s);
    unsigned long Tcurrent = config.alpha * T + (1.0 - config.alpha) * conn->bitrate;
    conn->bitrate = (int) Tcurrent;
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
    return send_data(conn->server.fd, buf, len);
}

static int handle_resp_html(proxy_conn_t *conn) {
    // forward response directly
}

static int handle_resp_f4m(proxy_conn_t *conn) {
    // 1. parse xml and get list of bitrates
    conn->bitrate_list = parse_xml_to_list(conn->server.request->post_body);
    // 2. send nolist version of request

}

static int handle_resp_f4m_nolist(proxy_conn_t *conn) {
    // forward response directly
}

static int handle_resp_chunk(proxy_conn_t *conn) {
    // 1. calculate and update throughput
    // 2. forward response
}

