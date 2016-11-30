#include "log.h"

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