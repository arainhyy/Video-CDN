#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include "proxy2.h"
#include "helper.h"

static FILE *log_fp = NULL;

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

void log_init(const char *log_name) {
    log_fp = fopen(log_name, "w+");
    if (!log_fp) {
        perror("log_init");
    }
}

void log_record(int time, const char *client_ip,
                const char *query_name, const char *response_ip) {
    if (!log_fp) {
        return;
    }
    int ret = fprintf(log_fp, "%d %s %s %s\n", time, client_ip, query_name, response_ip);
    if (ret < 0) {
        perror("log_record");
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
