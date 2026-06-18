#include "lynx_daemon.h"
#include "lynx_agent.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

int lynx_daemon_init(lynx_daemon_config_t *config) {
    (void)config;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    return 0;
}

void lynx_daemon_shutdown(void) {
    g_running = 0;
}

static int process_connection(lynx_connection_t *conn, lynx_frame_callback_t callback) {
    uint8_t tmp[65536];
    ssize_t n = read(conn->fd, tmp, sizeof(tmp));
    if (n <= 0) {
        return -1;
    }
    size_t nread = (size_t)n;

    if (conn->buf_len + nread > LYNX_DAEMON_BUF_SIZE) {
        conn->buf_len = 0;
    }
    memcpy(conn->buf + conn->buf_len, tmp, nread);
    conn->buf_len += nread;

    size_t offset = 0;
    while (offset < conn->buf_len) {
        lynx_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        int ret = lynx_frame_decode(conn->buf + offset, conn->buf_len - offset, &frame);
        if (ret == LYNX_CODEC_ERR_TRUNCATED) {
            break;
        }
        if (ret != LYNX_CODEC_OK) {
            offset++;
            continue;
        }

        if (frame.header.type == LYNX_FRAME_TYPE_METRICS
            && frame.payload != NULL
            && frame.header.payload_len >= 4) {
            uint32_t node_id_len = ((uint32_t)frame.payload[0] << 24)
                                 | ((uint32_t)frame.payload[1] << 16)
                                 | ((uint32_t)frame.payload[2] << 8)
                                 | (uint32_t)frame.payload[3];

            size_t min_payload = (size_t)4 + node_id_len + sizeof(lynx_metrics_t);
            if (node_id_len > 0
                && node_id_len < LYNX_DAEMON_MAX_NODE_ID
                && min_payload <= frame.header.payload_len) {
                memcpy(conn->node_id, frame.payload + 4, node_id_len);
                conn->node_id[node_id_len] = '\0';

                lynx_metrics_t metrics;
                memcpy(&metrics, frame.payload + 4 + node_id_len, sizeof(lynx_metrics_t));

                printf("[lynxd] metrics from '%s': cpu_user=%llu cpu_system=%llu cpu_idle=%llu "
                       "mem_total=%llu kB mem_avail=%llu kB "
                       "load=%.2f/%.2f/%.2f\n",
                       conn->node_id,
                       (unsigned long long)metrics.cpu_user,
                       (unsigned long long)metrics.cpu_system,
                       (unsigned long long)metrics.cpu_idle,
                       (unsigned long long)metrics.mem_total_kb,
                       (unsigned long long)metrics.mem_avail_kb,
                       metrics.load_1m, metrics.load_5m, metrics.load_15m);
            }
        } else if (frame.header.type != LYNX_FRAME_TYPE_METRICS) {
            printf("[lynxd] unhandled frame type %d\n", frame.header.type);
        }

        if (callback) {
            callback(&frame, conn->node_id);
        }

        size_t frame_size = sizeof(lynx_frame_header_t) + frame.header.payload_len;
        offset += frame_size;
        lynx_frame_free(&frame);
    }

    if (offset > 0 && offset < conn->buf_len) {
        memmove(conn->buf, conn->buf + offset, conn->buf_len - offset);
        conn->buf_len -= offset;
    } else if (offset >= conn->buf_len) {
        conn->buf_len = 0;
    }

    return 0;
}

int lynx_daemon_run(const lynx_daemon_config_t *config, lynx_frame_callback_t callback) {
    if (!config) return -1;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)config->port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    int actual_port = config->port;
    if (actual_port == 0) {
        struct sockaddr_in bound;
        socklen_t bound_len = sizeof(bound);
        if (getsockname(server_fd, (struct sockaddr *)&bound, &bound_len) == 0) {
            actual_port = ntohs(bound.sin_port);
        }
    }
    printf("[lynxd] listening on port %d\n", actual_port);

    lynx_connection_t connections[LYNX_DAEMON_MAX_CONNS];
    memset(connections, 0, sizeof(connections));
    for (int i = 0; i < LYNX_DAEMON_MAX_CONNS; i++) {
        connections[i].fd = -1;
        connections[i].buf = NULL;
        connections[i].buf_capacity = 0;
    }

    g_running = 1;

    while (g_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_fd = server_fd;

        for (int i = 0; i < LYNX_DAEMON_MAX_CONNS; i++) {
            if (connections[i].fd >= 0) {
                FD_SET(connections[i].fd, &readfds);
                if (connections[i].fd > max_fd) {
                    max_fd = connections[i].fd;
                }
            }
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue;

        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd,
                                   (struct sockaddr *)&client_addr,
                                   &client_len);
            if (client_fd < 0) continue;

            int slot = -1;
            for (int i = 0; i < LYNX_DAEMON_MAX_CONNS; i++) {
                if (connections[i].fd < 0) {
                    slot = i;
                    break;
                }
            }
            if (slot < 0) {
                printf("[lynxd] max connections reached, rejecting\n");
                close(client_fd);
                continue;
            }

            connections[slot].buf = (uint8_t *)malloc(LYNX_DAEMON_BUF_SIZE);
            connections[slot].buf_capacity = LYNX_DAEMON_BUF_SIZE;
            connections[slot].buf_len = 0;
            connections[slot].fd = client_fd;
            connections[slot].node_id[0] = '\0';
            printf("[lynxd] accepted connection on slot %d\n", slot);
            if (!connections[slot].buf) {
                printf("[lynxd] failed to allocate buffer for connection\n");
                close(client_fd);
                connections[slot].fd = -1;
                continue;
            }
        }

        for (int i = 0; i < LYNX_DAEMON_MAX_CONNS; i++) {
            if (connections[i].fd >= 0 && FD_ISSET(connections[i].fd, &readfds)) {
                if (process_connection(&connections[i], callback) < 0) {
                    printf("[lynxd] closing connection on slot %d\n", i);
                    close(connections[i].fd);
                    free(connections[i].buf);
                    connections[i].fd = -1;
                    connections[i].buf = NULL;
                    connections[i].buf_capacity = 0;
                    connections[i].buf_len = 0;
                    connections[i].node_id[0] = '\0';
                }
            }
        }
    }

    printf("[lynxd] shutting down\n");
    for (int i = 0; i < LYNX_DAEMON_MAX_CONNS; i++) {
        if (connections[i].fd >= 0) {
            close(connections[i].fd);
        }
        free(connections[i].buf);
    }
    close(server_fd);
    return 0;
}
