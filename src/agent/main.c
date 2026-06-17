#include "lynx_agent.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    lynx_agent_config_t config;
    config.daemon_host = "127.0.0.1";
    config.daemon_port = LYNX_AGENT_DEFAULT_PORT;
    config.interval_ms = LYNX_AGENT_COLLECT_INTERVAL_MS;
    config.node_id = "lynx-agent-1";

    int opt;
    while ((opt = getopt(argc, argv, "h:p:i:n:")) != -1) {
        switch (opt) {
        case 'h': config.daemon_host = optarg; break;
        case 'p': config.daemon_port = atoi(optarg); break;
        case 'i': config.interval_ms = atoi(optarg); break;
        case 'n': config.node_id = optarg; break;
        default:
            fprintf(stderr, "Usage: %s [-h host] [-p port]"
                            " [-i interval_ms] [-n node_id]\n", argv[0]);
            return 1;
        }
    }

    lynx_agent_run(&config);
    return 0;
}
