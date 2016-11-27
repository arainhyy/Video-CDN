#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <netinet/in.h>
#include <string.h>
#include "../../helper.h"

static const char *log_name = "logfile";

static void test_log(void **state) {
    log_init(log_name);
    log_record(5, "0.0.0.1" , "xxx.f4m", "0.0.0.2");
    log_record(6, "0.0.0.2" , "xxx.f4m", "0.0.0.3");
    log_close();
    unlink(log_name);
}

static void test_construct_http_req(void **state) {
    char reqstr[] = "GET xxx.html HTTP/1.1\r\nHost:localhost\r\nname2:val2\r\n\r\n";
    Request req;
    strcpy(req.http_method, "GET");
    strcpy(req.http_uri, "xxx.html");
    strcpy(req.http_version, "HTTP/1.1");
    struct Request_header header_1, header_2;
    strcpy(header_1.header_name, "Host");
    strcpy(header_1.header_value, "localhost");
    strcpy(header_2.header_name, "name2");
    strcpy(header_2.header_value, "val2");
    // construct headers list
    req.headers = &header_1;
    header_1.next = &header_2;
    header_2.next = NULL;
    // test construct
    char buf[8192];
    construct_http_req(buf, &req);
//    puts(buf);
    assert_memory_equal(buf, reqstr, strlen(reqstr));
}

int main(void) {
    const UnitTest tests[] = {
            unit_test(test_log),
            unit_test(test_construct_http_req),
    };
    return run_tests(tests);
}
