#include "lynx_daemon.h"
#include "lynx_codec.h"
#include "lynx_agent.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    printf("  TEST: %s ... ", name); \
    tests_run++; \
} while (0)

#define PASS() do { \
    printf("PASSED\n"); \
    tests_passed++; \
} while (0)

#define FAIL(msg) do { \
    printf("FAILED: %s\n", msg); \
} while (0)

static int find_free_port(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    socklen_t len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) < 0) {
        close(fd);
        return -1;
    }
    int port = ntohs(addr.sin_port);
    close(fd);
    return port;
}

struct daemon_test_arg {
    lynx_daemon_config_t config;
};

static void *run_daemon(void *arg) {
    daemon_test_arg *dta = (daemon_test_arg *)arg;
    lynx_daemon_init(&dta->config);
    lynx_daemon_run(&dta->config, NULL);
    delete dta;
    return NULL;
}

static void test_metrics_encode_decode(void) {
    TEST("metrics frame encode/decode roundtrip");

    lynx_metrics_t orig;
    memset(&orig, 0, sizeof(orig));
    orig.cpu_user = 12345;
    orig.cpu_system = 6789;
    orig.cpu_idle = 999999;
    orig.mem_total_kb = 16777216;
    orig.mem_avail_kb = 8388608;
    orig.load_1m = 0.85;
    orig.load_5m = 0.42;
    orig.load_15m = 0.33;

    const char *node_id = "test-node-01";
    uint32_t node_id_len = (uint32_t)strlen(node_id);
    uint32_t payload_len = sizeof(uint32_t) + node_id_len + sizeof(lynx_metrics_t);

    lynx_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    int rc = lynx_frame_init(&frame, LYNX_FRAME_TYPE_METRICS, payload_len);
    if (rc != LYNX_CODEC_OK) { FAIL("frame_init"); return; }

    frame.payload[0] = (uint8_t)(node_id_len >> 24);
    frame.payload[1] = (uint8_t)(node_id_len >> 16);
    frame.payload[2] = (uint8_t)(node_id_len >> 8);
    frame.payload[3] = (uint8_t)(node_id_len & 0xFF);
    memcpy(frame.payload + 4, node_id, node_id_len);
    memcpy(frame.payload + 4 + node_id_len, &orig, sizeof(lynx_metrics_t));
    frame.header.payload_len = payload_len;

    uint8_t *wire = NULL;
    size_t wire_len = 0;
    rc = lynx_frame_encode(&frame, &wire, &wire_len);
    if (rc != LYNX_CODEC_OK) { FAIL("encode"); lynx_frame_free(&frame); return; }

    lynx_frame_t decoded;
    memset(&decoded, 0, sizeof(decoded));
    rc = lynx_frame_decode(wire, wire_len, &decoded);
    if (rc != LYNX_CODEC_OK) { FAIL("decode"); free(wire); lynx_frame_free(&frame); return; }

    if (decoded.header.type != LYNX_FRAME_TYPE_METRICS) {
        FAIL("type mismatch"); goto cleanup;
    }

    {
        uint32_t d_nl = ((uint32_t)decoded.payload[0] << 24)
                      | ((uint32_t)decoded.payload[1] << 16)
                      | ((uint32_t)decoded.payload[2] << 8)
                      | (uint32_t)decoded.payload[3];
        if (d_nl != node_id_len) { FAIL("node_id_len mismatch"); goto cleanup; }

        char d_nid[64];
        memset(d_nid, 0, sizeof(d_nid));
        memcpy(d_nid, decoded.payload + 4, d_nl);

        lynx_metrics_t dm;
        memcpy(&dm, decoded.payload + 4 + d_nl, sizeof(lynx_metrics_t));

        if (dm.cpu_user != orig.cpu_user) { FAIL("cpu_user mismatch"); goto cleanup; }
        if (dm.cpu_system != orig.cpu_system) { FAIL("cpu_system mismatch"); goto cleanup; }
        if (dm.cpu_idle != orig.cpu_idle) { FAIL("cpu_idle mismatch"); goto cleanup; }
        if (dm.mem_total_kb != orig.mem_total_kb) { FAIL("mem_total_kb mismatch"); goto cleanup; }
        if (dm.mem_avail_kb != orig.mem_avail_kb) { FAIL("mem_avail_kb mismatch"); goto cleanup; }
        if (dm.load_1m != orig.load_1m) { FAIL("load_1m mismatch"); goto cleanup; }
        if (dm.load_5m != orig.load_5m) { FAIL("load_5m mismatch"); goto cleanup; }
        if (dm.load_15m != orig.load_15m) { FAIL("load_15m mismatch"); goto cleanup; }
    }

    PASS();

cleanup:
    free(wire);
    lynx_frame_free(&frame);
    lynx_frame_free(&decoded);
}

static void test_daemon_accept_and_process(void) {
    TEST("daemon accept and process metrics frame");

    int port = find_free_port();
    if (port <= 0) { FAIL("could not find free port"); return; }

    daemon_test_arg *arg = new daemon_test_arg();
    memset(arg, 0, sizeof(*arg));
    arg->config.port = port;

    pthread_t thr;
    if (pthread_create(&thr, NULL, run_daemon, arg) != 0) {
        delete arg;
        FAIL("pthread_create"); return;
    }

    usleep(200000);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { FAIL("socket"); pthread_cancel(thr); return; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        FAIL("connect"); close(sock); lynx_daemon_shutdown();
        pthread_join(thr, NULL); return;
    }

    lynx_metrics_t metrics;
    memset(&metrics, 0, sizeof(metrics));
    metrics.cpu_user = 42;
    metrics.cpu_system = 7;
    metrics.cpu_idle = 100;
    metrics.mem_total_kb = 8000000;
    metrics.mem_avail_kb = 4000000;
    metrics.load_1m = 0.5;
    metrics.load_5m = 0.3;
    metrics.load_15m = 0.1;

    const char *node_id = "test-daemon-node";
    uint32_t node_id_len = (uint32_t)strlen(node_id);
    uint32_t payload_len = sizeof(uint32_t) + node_id_len + sizeof(lynx_metrics_t);

    lynx_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    int rc = lynx_frame_init(&frame, LYNX_FRAME_TYPE_METRICS, payload_len);
    if (rc != LYNX_CODEC_OK) {
        FAIL("frame_init"); close(sock); lynx_daemon_shutdown();
        pthread_join(thr, NULL); return;
    }

    frame.payload[0] = (uint8_t)(node_id_len >> 24);
    frame.payload[1] = (uint8_t)(node_id_len >> 16);
    frame.payload[2] = (uint8_t)(node_id_len >> 8);
    frame.payload[3] = (uint8_t)(node_id_len & 0xFF);
    memcpy(frame.payload + 4, node_id, node_id_len);
    memcpy(frame.payload + 4 + node_id_len, &metrics, sizeof(lynx_metrics_t));
    frame.header.payload_len = payload_len;

    uint8_t *wire = NULL;
    size_t wire_len = 0;
    rc = lynx_frame_encode(&frame, &wire, &wire_len);
    if (rc != LYNX_CODEC_OK) {
        FAIL("encode"); close(sock); lynx_frame_free(&frame);
        lynx_daemon_shutdown(); pthread_join(thr, NULL); return;
    }

    ssize_t sent = send(sock, wire, wire_len, 0);
    bool ok = ((size_t)sent == wire_len);

    free(wire);
    lynx_frame_free(&frame);
    close(sock);

    usleep(100000);

    lynx_daemon_shutdown();
    pthread_join(thr, NULL);

    if (!ok) { FAIL("send failed"); return; }
    PASS();
}

static void test_daemon_shutdown_clean(void) {
    TEST("daemon shutdown without connections");

    daemon_test_arg *arg = new daemon_test_arg();
    memset(arg, 0, sizeof(*arg));
    arg->config.port = 0;

    lynx_daemon_init(&arg->config);

    pthread_t thr;
    if (pthread_create(&thr, NULL, run_daemon, arg) != 0) {
        delete arg;
        FAIL("pthread_create"); return;
    }

    usleep(100000);
    lynx_daemon_shutdown();
    pthread_join(thr, NULL);
    PASS();
}

int main(void) {
    printf("=== LynxLab Daemon Unit Tests ===\n\n");

    test_metrics_encode_decode();
    test_daemon_accept_and_process();
    test_daemon_shutdown_clean();

    printf("\nResults: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
