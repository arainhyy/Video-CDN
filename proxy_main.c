#include "proxy2.h"

int main(int argc, char **argv) {
    // arguments parsing
    if (argc == 7) {
        proxy_init_config(argv, 0);
    } else if (argc == 8) {
        proxy_init_config(argv, 1);
    } else {
        // optional: show arguments before exit
        return -1;
    }
    int ret = proxy_run();
    return ret;
}
