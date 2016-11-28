#include <stdio.h>
#include "proxy2.h"

int main(int argc, char **argv) {
    // arguments parsing
    if (argc == 7) {
        proxy_init_config(argv, 0);
    } else if (argc == 8) {
	    puts("Correct !");
        proxy_init_config(argv, 1);
    } else {
        // optional: show arguments before exit
        puts("Usage:");
        puts("./proxy <log> <alpha> <listen-port> <fake-ip> <dns-ip> <dns-port> <www-ip>");
        return -1;
    }
    int ret = proxy_run();
    return ret;
}
