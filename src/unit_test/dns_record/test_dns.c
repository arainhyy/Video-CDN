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
    parse_name(test_name, strlen(test_name) - 1, result);
//    printf("test string length: %d\n", strlen(test_name) - 1);
//    puts(result);
    assert_memory_equal(result, compare, strlen(compare));
}

static void test_translate_name(void **state) {
    char test_name[] = "www.baidu.com";
    char result[65535];
    char compare[65535];
    translate_name(test_name, result);
    parse_name(result, strlen(test_name), compare);
//    puts(compare);
    assert_memory_equal(compare, test_name, strlen(test_name));

}

int main(void) {
    const UnitTest tests[] = {
            unit_test(test_macro),
            unit_test(test_header_init),
            unit_test(test_parse_name),
            unit_test(test_translate_name),
    };
    return run_tests(tests);
}
