#include "lynx_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void test_frame_init_free(void)
{
    TEST("frame init/free");

    lynx_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    int rc = lynx_frame_init(&frame, LYNX_FRAME_TYPE_METRICS, 1024);
    if (rc != LYNX_CODEC_OK) { FAIL("init returned error"); return; }
    if (frame.header.magic != LYNX_CODEC_MAGIC) { FAIL("bad magic"); return; }
    if (frame.header.type != LYNX_FRAME_TYPE_METRICS) { FAIL("bad type"); return; }
    if (frame.payload == NULL) { FAIL("payload is NULL"); return; }
    if (frame.payload_capacity != 1024) { FAIL("bad capacity"); return; }
    if (!frame.owns_payload) { FAIL("owns_payload not set"); return; }

    lynx_frame_free(&frame);
    if (frame.payload != NULL) { FAIL("payload not NULL after free"); return; }
    PASS();
}

static void test_frame_init_zero_payload(void)
{
    TEST("frame init zero payload");

    lynx_frame_t frame;
    int rc = lynx_frame_init(&frame, LYNX_FRAME_TYPE_HEALTH, 0);
    if (rc != LYNX_CODEC_OK) { FAIL("init returned error"); return; }
    if (frame.payload != NULL) { FAIL("payload should be NULL for zero capacity"); return; }
    if (frame.payload_capacity != 0) { FAIL("capacity should be 0"); return; }

    lynx_frame_free(&frame);
    PASS();
}

static void test_frame_init_invalid_capacity(void)
{
    TEST("frame init invalid capacity");

    lynx_frame_t frame;
    int rc = lynx_frame_init(&frame, LYNX_FRAME_TYPE_METRICS, LYNX_MAX_PAYLOAD_SIZE + 1);
    if (rc != LYNX_CODEC_ERR_INVAL) { FAIL("should return INVAL for oversized capacity"); return; }

    rc = lynx_frame_init(NULL, LYNX_FRAME_TYPE_METRICS, 0);
    if (rc != LYNX_CODEC_ERR_INVAL) { FAIL("should return INVAL for NULL frame"); return; }

    PASS();
}

static void test_encode_decode_roundtrip(void)
{
    TEST("encode/decode roundtrip");

    const char *test_data = "hello lynxlab metrics payload";
    size_t data_len = strlen(test_data) + 1;

    lynx_frame_t tx;
    memset(&tx, 0, sizeof(tx));
    assert(lynx_frame_init(&tx, LYNX_FRAME_TYPE_METRICS, (uint32_t)data_len) == LYNX_CODEC_OK);
    memcpy(tx.payload, test_data, data_len);
    tx.header.payload_len = (uint32_t)data_len;
    tx.header.sequence = 42;
    tx.header.timestamp_ns = 171851234567890ULL;
    tx.header.flags = 0x01;

    uint8_t *wire = NULL;
    size_t wire_len = 0;
    int rc = lynx_frame_encode(&tx, &wire, &wire_len);
    if (rc != LYNX_CODEC_OK) { FAIL("encode failed"); lynx_frame_free(&tx); return; }
    if (wire == NULL) { FAIL("wire buffer is NULL"); lynx_frame_free(&tx); return; }
    if (wire_len != sizeof(lynx_frame_header_t) + data_len) {
        FAIL("wire length mismatch"); free(wire); lynx_frame_free(&tx); return;
    }

    lynx_frame_t rx;
    memset(&rx, 0, sizeof(rx));
    rc = lynx_frame_decode(wire, wire_len, &rx);
    if (rc != LYNX_CODEC_OK) { FAIL("decode failed"); free(wire); lynx_frame_free(&tx); return; }

    if (rx.header.type != LYNX_FRAME_TYPE_METRICS) { FAIL("type mismatch"); goto cleanup; }
    if (rx.header.sequence != 42) { FAIL("sequence mismatch"); goto cleanup; }
    if (rx.header.timestamp_ns != 171851234567890ULL) { FAIL("timestamp mismatch"); goto cleanup; }
    if (rx.header.payload_len != data_len) { FAIL("payload_len mismatch"); goto cleanup; }
    if (memcmp(rx.payload, test_data, data_len) != 0) { FAIL("payload data mismatch"); goto cleanup; }

    PASS();

cleanup:
    free(wire);
    lynx_frame_free(&tx);
    lynx_frame_free(&rx);
}

static void test_encode_decode_empty_payload(void)
{
    TEST("encode/decode empty payload");

    lynx_frame_t tx;
    memset(&tx, 0, sizeof(tx));
    assert(lynx_frame_init(&tx, LYNX_FRAME_TYPE_HEALTH, 0) == LYNX_CODEC_OK);
    tx.header.payload_len = 0;

    uint8_t *wire = NULL;
    size_t wire_len = 0;
    int rc = lynx_frame_encode(&tx, &wire, &wire_len);
    if (rc != LYNX_CODEC_OK) { FAIL("encode failed"); lynx_frame_free(&tx); return; }

    lynx_frame_t rx;
    memset(&rx, 0, sizeof(rx));
    rc = lynx_frame_decode(wire, wire_len, &rx);
    if (rc != LYNX_CODEC_OK) { FAIL("decode failed"); free(wire); lynx_frame_free(&tx); return; }
    if (rx.header.type != LYNX_FRAME_TYPE_HEALTH) { FAIL("type mismatch"); goto cleanup; }
    if (rx.header.payload_len != 0) { FAIL("payload_len should be 0"); goto cleanup; }
    if (rx.payload != NULL) { FAIL("payload should be NULL"); goto cleanup; }

    PASS();

cleanup:
    free(wire);
    lynx_frame_free(&tx);
    lynx_frame_free(&rx);
}

static void test_decode_tampered_checksum(void)
{
    TEST("decode tampered checksum");

    lynx_frame_t tx;
    memset(&tx, 0, sizeof(tx));
    assert(lynx_frame_init(&tx, LYNX_FRAME_TYPE_EVENT, 64) == LYNX_CODEC_OK);
    tx.header.payload_len = 0;

    uint8_t *wire = NULL;
    size_t wire_len = 0;
    assert(lynx_frame_encode(&tx, &wire, &wire_len) == LYNX_CODEC_OK);

    /* Corrupt the checksum field */
    wire[24] ^= 0xFF;

    lynx_frame_t rx;
    memset(&rx, 0, sizeof(rx));
    int rc = lynx_frame_decode(wire, wire_len, &rx);
    if (rc != LYNX_CODEC_ERR_CHECKSUM) {
        FAIL("should detect checksum error"); free(wire); lynx_frame_free(&tx); return;
    }

    PASS();
    free(wire);
    lynx_frame_free(&tx);
}

static void test_decode_truncated(void)
{
    TEST("decode truncated buffer");

    uint8_t buf[4] = {0};
    lynx_frame_t rx;
    memset(&rx, 0, sizeof(rx));
    int rc = lynx_frame_decode(buf, 4, &rx);
    if (rc != LYNX_CODEC_ERR_TRUNCATED) {
        FAIL("should detect truncated buffer"); return;
    }

    rc = lynx_frame_decode(NULL, 0, &rx);
    if (rc != LYNX_CODEC_ERR_INVAL) {
        FAIL("should return INVAL for NULL buf"); return;
    }

    PASS();
}

static void test_encoded_size(void)
{
    TEST("encoded size");

    lynx_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    assert(lynx_frame_init(&frame, LYNX_FRAME_TYPE_COMMAND, 100) == LYNX_CODEC_OK);
    frame.header.payload_len = 50;

    size_t sz = lynx_frame_encoded_size(&frame);
    if (sz != sizeof(lynx_frame_header_t) + 50) {
        FAIL("wrong encoded size"); lynx_frame_free(&frame); return;
    }

    sz = lynx_frame_encoded_size(NULL);
    if (sz != 0) { FAIL("NULL should return 0"); lynx_frame_free(&frame); return; }

    PASS();
    lynx_frame_free(&frame);
}

int main(void)
{
    printf("=== LynxLab Codec Unit Tests ===\n\n");

    test_frame_init_free();
    test_frame_init_zero_payload();
    test_frame_init_invalid_capacity();
    test_encode_decode_roundtrip();
    test_encode_decode_empty_payload();
    test_decode_tampered_checksum();
    test_decode_truncated();
    test_encoded_size();

    printf("\nResults: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
