#include "lynx_daemon.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-p port]\n", prog);
    fprintf(stderr, "  -p port  Listening port (default: %d)\n",
            LYNX_DAEMON_DEFAULT_PORT);
}

int main(int argc, char **argv) {
    lynx_daemon_config_t config;
    memset(&config, 0, sizeof(config));
    config.port = LYNX_DAEMON_DEFAULT_PORT;

    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
        case 'p':
            config.port = atoi(optarg);
            if (config.port <= 0 || config.port > 65535) {
                fprintf(stderr, "Invalid port: %s\n", optarg);
                return 1;
            }
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (lynx_daemon_init(&config) != 0) {
        fprintf(stderr, "Failed to initialize daemon\n");
        return 1;
    }

    printf("[lynxd] starting on port %d\n", config.port);
    fflush(stdout);

    int ret = lynx_daemon_run(&config, NULL);
    if (ret != 0) {
        fprintf(stderr, "Daemon exited with error\n");
        return 1;
    }

    return 0;
}
