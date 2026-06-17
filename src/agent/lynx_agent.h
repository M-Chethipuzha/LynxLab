#ifndef LYNX_AGENT_H
#define LYNX_AGENT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LYNX_AGENT_DEFAULT_PORT 9710
#define LYNX_AGENT_COLLECT_INTERVAL_MS 2000

typedef struct {
    const char *daemon_host;
    int daemon_port;
    int interval_ms;
    const char *node_id;
} lynx_agent_config_t;

typedef struct {
    uint64_t cpu_user;
    uint64_t cpu_system;
    uint64_t cpu_idle;
    uint64_t mem_total_kb;
    uint64_t mem_avail_kb;
    double load_1m;
    double load_5m;
    double load_15m;
} lynx_metrics_t;

int lynx_agent_collect_metrics(lynx_metrics_t *metrics);
int lynx_agent_send_metrics(int sockfd, const lynx_metrics_t *metrics, const char *node_id);
int lynx_agent_connect(const char *host, int port);
void lynx_agent_run(const lynx_agent_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* LYNX_AGENT_H */