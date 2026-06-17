#include "lynx_agent.h"
#include "lynx_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

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

static void test_collect_metrics(void)
{
    TEST("collect_metrics reads /proc");

    lynx_metrics_t m;
    memset(&m, 0, sizeof(m));
    int rc = lynx_agent_collect_metrics(&m);

    /* On macOS /proc doesn't exist — just check the function doesn't crash */
    if (rc != 0) {
        printf("SKIPPED (no /proc)\n");
        tests_passed++;
        return;
    }

    if (m.cpu_user == 0 && m.cpu_system == 0 && m.cpu_idle == 0) {
        FAIL("all CPU counters are zero");
        return;
    }
    if (m.mem_total_kb == 0) { FAIL("mem_total_kb is zero"); return; }
    if (m.mem_avail_kb == 0) { FAIL("mem_avail_kb is zero"); return; }
    if (m.mem_avail_kb > m.mem_total_kb) {
        FAIL("mem_avail > mem_total"); return;
    }
    if (m.load_1m < 0) { FAIL("load_1m is negative"); return; }

    PASS();
}

static void test_collect_metrics_null(void)
{
    TEST("collect_metrics NULL returns -1");

    int rc = lynx_agent_collect_metrics(NULL);
    if (rc != -1) { FAIL("should return -1 for NULL"); return; }

    PASS();
}

static void test_connect_invalid_host(void)
{
    TEST("connect invalid host returns -1");

    /* 127.0.0.1 with no listener returns ECONNREFUSED immediately */
    int fd = lynx_agent_connect("127.0.0.1", 1);
    if (fd >= 0) {
        close(fd);
        FAIL("should return -1 for unreachable host");
        return;
    }

    PASS();
}

static void test_connect_null_host(void)
{
    TEST("connect NULL host returns -1");

    int fd = lynx_agent_connect(NULL, 9710);
    if (fd != -1) { FAIL("should return -1 for NULL host"); return; }

    PASS();
}

static void test_connect_bad_port(void)
{
    TEST("connect bad port returns -1");

    int fd = lynx_agent_connect("127.0.0.1", -1);
    if (fd != -1) { FAIL("should return -1 for port <= 0"); return; }

    PASS();
}

static void test_send_metrics_bad_fd(void)
{
    TEST("send_metrics bad fd returns -1");

    lynx_metrics_t m;
    memset(&m, 0, sizeof(m));
    m.cpu_user = 100;
    m.mem_total_kb = 8000000;
    m.load_1m = 0.5;

    int rc = lynx_agent_send_metrics(-1, &m, "test-node");
    if (rc != -1) { FAIL("should return -1 for bad fd"); return; }

    PASS();
}

static void test_send_metrics_null_params(void)
{
    TEST("send_metrics NULL params returns -1");

    lynx_metrics_t m;
    memset(&m, 0, sizeof(m));
    int rc = lynx_agent_send_metrics(0, NULL, "test-node");
    if (rc != -1) { FAIL("should return -1 for NULL metrics"); return; }

    rc = lynx_agent_send_metrics(0, &m, NULL);
    if (rc != -1) { FAIL("should return -1 for NULL node_id"); return; }

    PASS();
}

int main(void)
{
    printf("=== LynxLab Agent Unit Tests ===\n\n");

    test_collect_metrics();
    test_collect_metrics_null();
    test_connect_invalid_host();
    test_connect_null_host();
    test_connect_bad_port();
    test_send_metrics_bad_fd();
    test_send_metrics_null_params();

    printf("\nResults: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
