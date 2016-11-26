#ifndef __COMMON_H__
#define __COMMON_H__

#define MAX_PATH_LEN (512)  // log file path length
#define MAX_REQ_SIZE (4096) // http request limit

typedef enum req_uri {
	HTML,
	F4M,
	CHUNK,
	UK,
} req_uri_t;

typedef enum resp_uri {
	HTML,
	F4M,
	CHUNK,
	UK,
} resp_uri_t;

#endif /* !__COMMON_H__ */