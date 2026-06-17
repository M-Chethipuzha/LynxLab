#include "lynx_agent.h"
#include "lynx_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

int lynx_agent_collect_metrics(lynx_metrics_t *metrics)
{
    if (!metrics) return -1;

    FILE *f;
    char line[256];
    int found = 0;

    f = fopen("/proc/stat", "r");
    if (!f) return -1;
    if (fgets(line, sizeof(line), f)) {
        unsigned long long user, nice, system, idle;
        if (sscanf(line, "cpu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle) >= 4) {
            metrics->cpu_user   = user;
            metrics->cpu_system = system;
            metrics->cpu_idle   = idle;
            found = 1;
        }
    }
    fclose(f);
    if (!found) return -1;

    f = fopen("/proc/meminfo", "r");
    if (!f) return -1;
    found = 0;
    while (fgets(line, sizeof(line), f)) {
        unsigned long long val;
        if (sscanf(line, "MemTotal: %llu", &val) == 1) {
            metrics->mem_total_kb = val;
            found++;
        } else if (sscanf(line, "MemAvailable: %llu", &val) == 1) {
            metrics->mem_avail_kb = val;
            found++;
        }
    }
    fclose(f);
    if (found != 2) return -1;

    f = fopen("/proc/loadavg", "r");
    if (!f) return -1;
    if (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%lf %lf %lf",
                   &metrics->load_1m,
                   &metrics->load_5m,
                   &metrics->load_15m) != 3) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);

    return 0;
}

int lynx_agent_connect(const char *host, int port)
{
    if (!host || port <= 0) return -1;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int lynx_agent_send_metrics(int sockfd, const lynx_metrics_t *metrics,
                            const char *node_id)
{
    if (sockfd < 0 || !metrics || !node_id) return -1;

    size_t node_id_len = strlen(node_id);
    if (node_id_len > UINT32_MAX) return -1;

    uint32_t payload_len = (uint32_t)(sizeof(uint32_t) + node_id_len
                                      + sizeof(lynx_metrics_t));

    lynx_frame_t frame;
    int ret = lynx_frame_init(&frame, LYNX_FRAME_TYPE_METRICS, payload_len);
    if (ret != LYNX_CODEC_OK) return -1;

    /* Payload: [4B node_id_len][node_id bytes][raw lynx_metrics_t] */
    uint8_t *p = frame.payload;
    p[0] = (uint8_t)(node_id_len >> 24);
    p[1] = (uint8_t)(node_id_len >> 16);
    p[2] = (uint8_t)(node_id_len >> 8);
    p[3] = (uint8_t)(node_id_len & 0xFF);
    memcpy(p + 4, node_id, node_id_len);
    memcpy(p + 4 + node_id_len, metrics, sizeof(lynx_metrics_t));

    frame.header.payload_len = payload_len;

    uint8_t *wire_buf = NULL;
    size_t wire_len = 0;
    ret = lynx_frame_encode(&frame, &wire_buf, &wire_len);
    if (ret != LYNX_CODEC_OK) {
        lynx_frame_free(&frame);
        return -1;
    }

    ssize_t sent = send(sockfd, wire_buf, wire_len, 0);
    int ok = ((size_t)sent == wire_len) ? 0 : -1;

    free(wire_buf);
    lynx_frame_free(&frame);
    return ok;
}

void lynx_agent_run(const lynx_agent_config_t *config)
{
    if (!config) return;

    lynx_metrics_t metrics;
    int sockfd = -1;

    while (1) {
        if (lynx_agent_collect_metrics(&metrics) != 0) {
            goto sleep;
        }

        if (sockfd < 0) {
            sockfd = lynx_agent_connect(config->daemon_host,
                                        config->daemon_port);
        }

        if (sockfd >= 0) {
            if (lynx_agent_send_metrics(sockfd, &metrics,
                                        config->node_id) != 0) {
                close(sockfd);
                sockfd = -1;
            }
        }

sleep:
        if (config->interval_ms <= 0) break;

        struct timespec ts;
        ts.tv_sec = config->interval_ms / 1000;
        ts.tv_nsec = (long)(config->interval_ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }

    if (sockfd >= 0) close(sockfd);
}
