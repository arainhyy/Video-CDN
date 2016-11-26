#ifndef __HELPER_H__
#define __HELPER_H__

// encapsulation of sending data and error handling
// return 0 on succ, neg on error
// used to send response to browser and request to server
// maybe should move this function to proxy module?? since currently
// no other module use this...
int send_data(int fd, char *buf, int len);

// log record
void log_record(int time, const char *client_ip,
	const char *query_name, const char *response_ip);
void log_init(const char *log_name);

#endif /* !__HELPER_H__ */