#include "parse.h"

/**
* Given a char buffer returns the parsed request headers
*/
Request *parse(char *buffer, int size) {
    //Differant states in the state machine
    enum {
        STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
    };

    int i = 0, state;
    size_t offset = 0;
    char ch;
    char buf[8192];
    memset(buf, 0, 8192);
    state = STATE_START;
    while (state != STATE_CRLFCRLF) {
        char expected = 0;

        if (i == size)
            break;

        ch = buffer[i++];
        buf[offset++] = ch;

        switch (state) {
            case STATE_START:
            case STATE_CRLF:
                expected = '\r';
                break;
            case STATE_CR:
            case STATE_CRLFCR:
                expected = '\n';
                break;
            default:
                state = STATE_START;
                continue;
        }

        if (ch == expected)
            state++;
        else
            state = STATE_START;
    }
    Request *request = (Request *) malloc(sizeof(Request));
    if (!request) {
        perror("parse malloc");
        return NULL;
    }
    request->header_count = 0;
    request->content_length = 0;
    request->content_readed = 0;
    request->position = i;
    request->headers = NULL;
    strcpy(request->http_uri, "");
    strcpy(request->http_method, "");
    strcpy(request->http_uri, "");
  	// Valid End State
	if (state == STATE_CRLFCRLF) {
		set_parsing_options(buf, i, request);
		request->status = 0;
		if (yyparse() == SUCCESS) {
      		return request;
		} else {
			request->status = 400;
		}
	} else {
		request->status = NEEDMORE;
	}
  	// Handle Malformed Requests
  	return request;
}

/**
 * Check type by request struct.
 *
 * @param request Parsed request struct.
 * @return request enum type.
 */
int check_type(Request *request) {
    if (strcasestr(request->http_uri, ".f4m") != NULL) {
        return REQ_F4M;
    }
    if (strcasestr(request->http_uri, ".html") != NULL) {
        return REQ_HTML;
    }
    //TODO(yayunh): temporarily use "-Frag" to check chunk request.
    if (strcasestr(request->http_uri, "-Frag") != NULL) {
        return REQ_CHUNK;
    }
    return REQ_UK;
}

/**
 * Replace a f4m request to nolist f4m request. Called this function to change request after sending
 * f4m request.
 *
 * @param f4m_request
 * @return IS_F4M or IS_NOT_F4M enum value. IS_F4M means successful replacement.
 */
int replace_f4m_to_nolist(char *f4m_request) {
    char *pt;
    if ((pt = strstr(f4m_request, ".f4m ")) == NULL) {
        return IS_NOT_F4M;
    }
    int length = strlen(f4m_request) - (pt - f4m_request);
    memmove(pt + strlen("_nolist.f4m "), pt + strlen(".f4m "), length);
    memmove(pt, "_nolist.f4m ", strlen("_nolist.f4m "));
    return IS_F4M;
}

/**
 * Replace fragment request with updated bitrate according to throughput.
 * @param request
 * @param bitrate
 * @return IS_FRAGMENT_REQUEST means successful replacement.
 */
int replace_uri_bitrate(char *request, int bitrate) {
    char *pt = NULL;
    if ((pt = strcasestr(request, " HTTP")) == NULL) {
        return IS_NOT_FRAGMENT_REQUEST;
    }
    char *left = NULL;
    if ((left = strcasestr(request, "seg")) == NULL) {
        return IS_NOT_FRAGMENT_REQUEST;
    }
    while (*pt != '/') pt--;
    *pt = '\0';
    char new_request[MAX_REQ_SIZE];
    snprintf(new_request, MAX_REQ_SIZE, "%s/%d%s", request, bitrate, left);
    strcpy(request, new_request);
    return IS_FRAGMENT_REQUEST;
}