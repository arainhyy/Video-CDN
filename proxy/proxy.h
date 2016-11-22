//
// Created by yayunh on 11/22/16.
//

#ifndef VIDEO_CDN_PROXY_H
#define VIDEO_CDN_PROXY_H
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "log.h"

// #define DEBUG
#ifdef DEBUG
# define dbg_printf(...) fprintf(stderr, __VA_ARGS__)
#else
# define dbg_printf(...)
#endif

#define PATH_LEN (2048)
#define MAX_CONNECTION_NUM (2000)
#define IP_LEN (1024)

#endif //VIDEO_CDN_PROXY_H
