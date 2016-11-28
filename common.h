#ifndef __COMMON_H__
#define __COMMON_H__

#define MAX_PATH_LEN (512)  // log file path length
#define MAX_REQ_SIZE (8192) // http request limit

#define IS_F4M (1)
#define IS_NOT_F4M (0)

#define IS_FRAGMENT_REQUEST (1)
#define IS_NOT_FRAGMENT_REQUEST (0)

typedef enum req_uri {
	REQ_HTML,
	REQ_F4M,
	REQ_CHUNK,
	REQ_UK,
} req_uri_t;

typedef enum resp_uri {
	RESP_HTML,
	RESP_F4M,
	RESP_F4M_NOLIST,
	RESP_CHUNK,
	RESP_UK,
} resp_uri_t;

#endif /* !__COMMON_H__ */
