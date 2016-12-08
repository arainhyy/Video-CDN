#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <netinet/in.h>
#include <string.h>
#include "../../dns_record.h"

static void test_macro(void **state) {

}

static void test_header_init(void **state) {
    dns_header_t header;
    int id = 233;
    dns_header_init(&header, id);
    assert_int_equal(DNS_GET_ID(&header), id);
}

static void test_parse_name(void **state) {
    char *compare = "www.baidu.com";
    char test_name[] = ".www.baidu.com";
    test_name[0] = 03;
    test_name[4] = 05;
    test_name[10] = 03;
    char result[65535];
    int ret = parse_name(test_name, strlen(test_name) - 1, result);
//    printf("test string length: %d\n", strlen(test_name) - 1);
//    puts(result);
    assert_memory_equal(result, compare, strlen(compare));
    assert_int_equal(ret, strlen(compare) + 1);
}

static void test_translate_name(void **state) {
    char test_name[] = "www.baidu.com";
    char result[65535];
    char compare[65535];
    int ret = translate_name(test_name, result);
    parse_name(result, strlen(test_name), compare);
//    puts(compare);
    assert_memory_equal(compare, test_name, strlen(test_name));
    assert_int_equal(ret, strlen(test_name));
}

static void test_parse_question(void **state) {
    char buf[65535];
    char result[65535];
    //translate_name()
    //int parse_question(const char *buf, int size, char *result);
    const char *addr = "www.baidu.com";
    int offset = translate_name(addr, buf);
    dns_question_t question;
    question.qclass = htons(1);
    question.qtype = htons(1);
    memcpy(buf + offset + 1, &question, sizeof(question));
    // parse
    int ret = parse_question(buf, offset + sizeof(question), result);
//    puts(result);
    assert_memory_equal(result, addr, strlen(addr));
    assert_int_equal(ret, offset + 1 + sizeof(question));
}

static void test_parse_resourse(void **state) {
    char buf[65535];
    char result[65535];
    //translate_name()
    //int parse_question(const char *buf, int size, char *result);
    const char *addr = "www.baidu.com";
    int offset = translate_name(addr, buf);
    dns_question_t question;
    question.qclass = htons(1);
    question.qtype = htons(1);
    memcpy(buf + offset + 1, &question, sizeof(question));
    // parse
    int ret = parse_question(buf, offset + sizeof(question), result);
//    puts(result);
    assert_memory_equal(result, addr, strlen(addr));
    assert_int_equal(ret, offset + 1 + sizeof(question));

}

int main(void) {
    const UnitTest tests[] = {
            unit_test(test_macro),
            unit_test(test_header_init),
            unit_test(test_parse_name),
            unit_test(test_translate_name),
            unit_test(test_parse_question),
    };
    return run_tests(tests);
}
