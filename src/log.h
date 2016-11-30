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