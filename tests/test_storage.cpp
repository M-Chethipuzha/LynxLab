#include "lynx_storage.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

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

static void test_store_and_query_single_node(void) {
    TEST("store and query metrics for a single node");

    LynxStorage storage(":memory:");

    int ret = storage.store_metrics("node-a", 1000, 101, 102, 103,
                                    8000000, 4000000, 1.5, 1.2, 1.0);
    if (ret != 0) { FAIL("store_metrics returned error"); return; }

    ret = storage.store_metrics("node-a", 2000, 201, 202, 203,
                                9000000, 5000000, 2.5, 2.2, 2.0);
    if (ret != 0) { FAIL("store_metrics 2 returned error"); return; }

    ret = storage.store_metrics("node-a", 3000, 301, 302, 303,
                                10000000, 6000000, 3.5, 3.2, 3.0);
    if (ret != 0) { FAIL("store_metrics 3 returned error"); return; }

    auto results = storage.query_recent("node-a", 10);
    if (results.size() != 3) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected 3 results, got %zu", results.size());
        FAIL(buf);
        return;
    }

    if (results[0].timestamp_ns != 3000) { FAIL("expected first result ts=3000"); return; }
    if (results[1].timestamp_ns != 2000) { FAIL("expected second result ts=2000"); return; }
    if (results[2].timestamp_ns != 1000) { FAIL("expected third result ts=1000"); return; }

    if (results[0].node_id != "node-a") { FAIL("node_id mismatch"); return; }
    if (results[0].cpu_user != 301) { FAIL("cpu_user mismatch"); return; }
    if (results[0].cpu_system != 302) { FAIL("cpu_system mismatch"); return; }
    if (results[0].cpu_idle != 303) { FAIL("cpu_idle mismatch"); return; }
    if (results[0].mem_total_kb != 10000000) { FAIL("mem_total_kb mismatch"); return; }
    if (results[0].mem_avail_kb != 6000000) { FAIL("mem_avail_kb mismatch"); return; }
    if (fabs(results[0].load_1m - 3.5) > 0.001) { FAIL("load_1m mismatch"); return; }
    if (fabs(results[0].load_5m - 3.2) > 0.001) { FAIL("load_5m mismatch"); return; }
    if (fabs(results[0].load_15m - 3.0) > 0.001) { FAIL("load_15m mismatch"); return; }

    PASS();
}

static void test_query_multi_node(void) {
    TEST("query metrics for multiple nodes separately");

    LynxStorage storage(":memory:");

    storage.store_metrics("node-x", 100, 1, 1, 1, 1000, 500, 0.1, 0.1, 0.1);
    storage.store_metrics("node-y", 200, 2, 2, 2, 2000, 1000, 0.2, 0.2, 0.2);
    storage.store_metrics("node-x", 300, 3, 3, 3, 3000, 1500, 0.3, 0.3, 0.3);

    auto x_results = storage.query_recent("node-x", 10);
    if (x_results.size() != 2) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected 2 for node-x, got %zu", x_results.size());
        FAIL(buf);
        return;
    }

    auto y_results = storage.query_recent("node-y", 10);
    if (y_results.size() != 1) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected 1 for node-y, got %zu", y_results.size());
        FAIL(buf);
        return;
    }

    if (x_results[0].timestamp_ns != 300) { FAIL("node-x first should be ts=300"); return; }
    if (x_results[1].timestamp_ns != 100) { FAIL("node-x second should be ts=100"); return; }

    PASS();
}

static void test_query_limit(void) {
    TEST("query respects limit parameter");

    LynxStorage storage(":memory:");

    for (int i = 1; i <= 10; i++) {
        storage.store_metrics("limited-node", i * 1000, i, i, i, i * 1000, i * 500,
                              (double)i, (double)i, (double)i);
    }

    auto all = storage.query_recent("limited-node", 100);
    if (all.size() != 10) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected 10, got %zu", all.size());
        FAIL(buf);
        return;
    }

    auto limited = storage.query_recent("limited-node", 3);
    if (limited.size() != 3) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected 3, got %zu", limited.size());
        FAIL(buf);
        return;
    }

    if (limited[0].timestamp_ns != 10000) { FAIL("limit first should be ts=10000"); return; }
    if (limited[1].timestamp_ns != 9000) { FAIL("limit second should be ts=9000"); return; }
    if (limited[2].timestamp_ns != 8000) { FAIL("limit third should be ts=8000"); return; }

    PASS();
}

static void test_query_nonexistent_node(void) {
    TEST("query for non-existent node returns empty");

    LynxStorage storage(":memory:");
    storage.store_metrics("real-node", 100, 1, 1, 1, 1000, 500, 0.1, 0.1, 0.1);

    auto results = storage.query_recent("ghost-node", 10);
    if (!results.empty()) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected empty, got %zu", results.size());
        FAIL(buf);
        return;
    }

    PASS();
}

int main(void) {
    printf("=== LynxLab Storage Unit Tests ===\n\n");

    test_store_and_query_single_node();
    test_query_multi_node();
    test_query_limit();
    test_query_nonexistent_node();

    printf("\nResults: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
