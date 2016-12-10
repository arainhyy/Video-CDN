#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>

FILE* log_fd;

int start_logger(char* file_name);
int close_logger();
void logout(const char* format, ...);
void log_init(const char *log_name);
void log_record(const char *log_name, int time, const char *client_ip,
                const char *query_name, const char *response_ip);