#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include "proxy2.h"
#include "helper.h"

static FILE *log_fp = NULL;
static const char *space = " ";
static const char *crlf = "\r\n";


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
    return total_sent;
}

void log_init(const char *log_name) {
    log_fp = fopen(log_name, "w+");
    if (!log_fp) {
        perror("log_init");
    }
}

//void log_record(int time, const char *client_ip,
//                const char *query_name, const char *response_ip) {
//    if (!log_fp) {
//        return;
//    }
//    int ret = fprintf(log_fp, "%d %s %s %s\n", time, client_ip, query_name, response_ip);
//    if (ret < 0) {
//        perror("log_record");
//    }
//}
void log_record(const char *log_name, unsigned long time, float duration, unsigned long t_put, unsigned long avg_tput,
                int bitrate, const char *server_ip, const char *chunk_name) {
    log_init(log_name);
    if (!log_fp) {
        return;
    }
	puts("log content");
    printf("%lu %.6f %lu %lu %d %s %s\n", time, duration, t_put, avg_tput, bitrate, server_ip,
                      chunk_name);
    int ret = fprintf(log_fp, "%lu %.6f %lu %lu %d %s %s\n", time, duration, t_put, avg_tput, bitrate, server_ip,
                      chunk_name);
    if (ret < 0) {
        perror("log_record");
    }
    if (log_fp != NULL) {
        fclose(log_fp);
    }
}

void log_close() {
    if (!log_fp) {
        return;
    }
    if (fclose(log_fp) < 0) {
        perror("log_close");
    }
}

int construct_http_req(char *buf, Request *req) {
    // request line
    buf[0] = '\0';
    strcat(buf, req->http_method); strcat(buf, space);
    strcat(buf, req->http_uri);    strcat(buf, space);
    strcat(buf, req->http_version);
//    printf("1 len: %d\n", strlen(buf));
    strcat(buf, crlf);
//    printf("2 len: %d\n", strlen(buf));
    // append list of header
    struct Request_header *header = req->headers;
    while (header) {
        strcat(buf, header->header_name);
        strcat(buf, ":");
        strcat(buf, header->header_value);
        strcat(buf, crlf);
        header = header->next;
    }
    strcat(buf, crlf);
    return strlen(buf);
}

int get_video_name(const char* uri, char* video_name) {
    strcpy(video_name, uri);
}