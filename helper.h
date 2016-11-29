#ifndef __HELPER_H__
#define __HELPER_H__

#include "parse.h"

// encapsulation of sending data and error handling
// return 0 on succ, neg on error
// used to send response to browser and request to server
// maybe should move this function to proxy module?? since currently
// no other module use this...
int send_data(int fd, char *buf, int len);

// log record
//void log_record(int time, const char *client_ip,
//                const char *query_name, const char *response_ip);
void log_record(const char *log_name, unsigned long time, float duration, unsigned long t_put, unsigned long avg_tput,
                int bitrate, const char *server_ip, const char *chunk_name);

void log_init(const char *log_name);

void log_close();

int construct_http_req(char *buf, Request *req);

int get_video_name(const char* uri, char* video_name);

#endif /* !__HELPER_H__ */