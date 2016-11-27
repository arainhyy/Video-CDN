#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <netinet/in.h>
#include "../../helper.h"

static const char *log_name = "logfile";

static void test_log(void **state) {
    log_init(log_name);
    log_record(5, "0.0.0.1" , "xxx.f4m", "0.0.0.2");
    log_record(6, "0.0.0.2" , "xxx.f4m", "0.0.0.3");
    log_close();
    unlink(log_name);
}

int main(void) {
    const UnitTest tests[] = {
            unit_test(test_log),
    };
    return run_tests(tests);
}
