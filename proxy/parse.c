#include "parse.h"

/**
* Given a char buffer returns the parsed request headers
*/
Request * parse(char *buffer, int size, int socketFd) {
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
	request->header_count = 0;
	request->is_cgi = 0;
	request->content_length = 0;
	request->content_readed = 0;
	request->position = i;
    request->headers = NULL;
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
