//
// Created by Hanlin Shi on 11/28/16.
//

static int handler_server2(proxy_conn_t *conn) {
    // read from socket
    puts("handle server");
    int recvlen = recv(conn->server.fd, conn->server.buf + conn->server.offset,
                       MAX_REQ_SIZE - conn->server.offset, MSG_DONTWAIT);
    if (recvlen < 0) {
        perror("handler_server recv");
        return -1;
    }
    conn->server.offset += recvlen;
    int ret = 0;
    while (conn->server.offset > 0 && conn->server.request->status != NEEDMORE && ret != -1) {
        // check if it has sent all content body of last request to client.
        if (conn->server.to_send_length > 0) {
            int to_send = conn->server.to_send_length > conn->server.offset ?
                          conn->server.offset : conn->server.to_send_length;
//        int send_ret = send_data(conn->browser.fd, conn->server.buf, to_send);
            int old_len_body = strlen(conn->server.response_body);
            memmove(conn->server.response_body + old_len_body, conn->server.buf, to_send);
            conn->server.response_body[old_len_body + to_send] = '\0';
            conn->server.to_send_length -= to_send;
            // Update buffer and offset.
            conn->server.offset -= to_send;
            if (conn->server.offset > 0) {
                memmove(conn->server.buf, conn->server.buf + to_send, conn->server.offset);
            }
        }
        if (conn->server.request != NULL && conn->server.request->status != NEEDMORE && conn->server.to_send_length == 0) {
            switch (conn->state) {
                case HTML:
                    ret = handle_resp_html(conn);
                    break;
                case F4M_NOLIST:
                    ret = handle_resp_html(conn);
                    break;
                case F4M:
                    ret = handle_resp_f4m(conn);
                    break;
                case CHUNK:
                    ret = handle_resp_chunk(conn);
                    break;
                default:ret = -1;
            }
//            estimate_throughput(conn, conn->server.request->content_length);
            free(conn->server.request);
            conn->server.request = NULL;
        }

        if (conn->server.offset <= 0) break;
        // parse request.
        conn->server.request = parse_reponse(conn->server.buf, recvlen);
        if (conn->server.request->status < 0) {
            printf("Incomplete request---------------\n");
            return 0;
        }
        conn->server.to_send_length = conn->server.request->content_length;
        conn->server.response_body = malloc(sizeof(char) * (conn->server.to_send_length + 1));
        conn->server.response_body = "";

        conn->server.offset -= conn->server.request->position;
        // Store server response before clear server receiving buffer.

        memcpy(conn->server.response, conn->server.buf, conn->server.request->position);
        conn->server.response[conn->server.request->position] = '\0';
        if (conn->server.offset > 0) {
            memmove(conn->server.buf, conn->server.buf + conn->server.request->position,
                    conn->server.offset);
        }
    }
    return ret;
}

static int handle_resp_html(proxy_conn_t *conn, const char *response) {
    // forward response directly
    return send_data(conn->browser.fd, response, strlen(response));
}

static int handle_resp_f4m(proxy_conn_t *conn, const char *response) {
    // 1. parse xml and get list of bitrates
    conn->bitrate_list = parse_xml_to_list(conn->server.request->post_body);
    // 2. request for nolist version
    replace_f4m_to_nolist(conn->server.f4m_request);
    int ret = send_data(conn->server.fd, conn->server.f4m_request, strlen(conn->server.f4m_request));
    puts("send f4m nolist request");
    // 3. set state
    conn->state = F4M_NOLIST;
    return ret;
}

static int handle_resp_f4m_nolist(proxy_conn_t *conn, const char *response) {
    // forward response directly
    int ret = send_data(conn->browser.fd, response, strlen(response));
    return ret;
}

static int handle_resp_chunk(proxy_conn_t *conn, const char *response) {
    // 1. calculate and update throughput
    float t_old = conn->T_curr;
    float t_est;
    // estimate_throughput(conn, conn->server.) // TODO
    // 2. forward response
    send_data(conn->browser.fd, response, strlen(response));
    // 3. log to file
    //log_record(get_mill_time() / 1000, ); // TODO
}
