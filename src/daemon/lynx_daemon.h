#ifndef LYNX_DAEMON_H
#define LYNX_DAEMON_H

#include <stddef.h>
#include <stdint.h>

#include "lynx_codec.h"

#define LYNX_DAEMON_DEFAULT_PORT 9710
#define LYNX_DAEMON_MAX_NODE_ID 64
#define LYNX_DAEMON_BUF_SIZE (1024 * 1024)
#define LYNX_DAEMON_MAX_CONNS 16

typedef struct {
    int port;
} lynx_daemon_config_t;

typedef struct {
    int fd;
    char node_id[LYNX_DAEMON_MAX_NODE_ID];
    uint8_t *buf;
    size_t buf_capacity;
    size_t buf_len;
} lynx_connection_t;

typedef void (*lynx_frame_callback_t)(const lynx_frame_t *frame, const char *node_id);

int lynx_daemon_init(lynx_daemon_config_t *config);
void lynx_daemon_shutdown(void);
int lynx_daemon_run(const lynx_daemon_config_t *config, lynx_frame_callback_t callback);

#endif /* LYNX_DAEMON_H */
