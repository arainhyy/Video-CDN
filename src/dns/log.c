#include "log.h"

static FILE *log_fp = NULL;
void log_init(const char *log_name);
void log_close();

int start_logger(char* file_name) {
    log_fd = fopen(file_name, "w");
    if (log_fd == NULL) {
        fprintf(stderr, "Can not start logger.\n");
        return 1;
    }
    return 0;
}

int close_logger() {
    if (log_fd != NULL) {
        fclose(log_fd);
        return 0;
    }
    fprintf(stderr, "Wrong log file descriptor.\n");
    return 1;
}

void logout(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    vfprintf(log_fd, format, arg);
    va_end(arg);
    fflush(log_fd);
}

void log_record(const char *log_name, int time, const char *client_ip,
                const char *query_name, const char *response_ip) {
    log_init(log_name);
    if (!log_fp) {
        return;
    }
    int ret = fprintf(log_fp, "%d %s %s %s\n", time, client_ip, query_name, response_ip);
    if (ret < 0) {
        perror("log_record");
    }
    if (log_fp != NULL) {
        fclose(log_fp);
    }
}

void log_init(const char *log_name) {
    log_fp = fopen(log_name, "a+");
    if (!log_fp) {
        perror("log_init");
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