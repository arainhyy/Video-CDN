#ifndef __COMMON_H__
#define __COMMON_H__

#define MAX_PATH_LEN (512)  // log file path length
#define MAX_REQ_SIZE (4096) // http request limit

typedef enum req_uri {
	REQ_HTML,
	REQ_F4M,
	REQ_CHUNK,
	REQ_UK,
} req_uri_t;

typedef enum resp_uri {
	RESP_HTML,
	RESP_F4M,
	RESP_CHUNK,
	RESP_UK,
} resp_uri_t;

#endif /* !__COMMON_H__ */
