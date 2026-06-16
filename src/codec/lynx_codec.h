#ifndef LYNX_CODEC_H
#define LYNX_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LYNX_CODEC_MAGIC 0x4C584E58  /* "LXNX" */
#define LYNX_CODEC_VERSION 1
#define LYNX_MAX_PAYLOAD_SIZE (1024 * 1024)  /* 1 MB max payload */

typedef enum {
    LYNX_FRAME_TYPE_METRICS = 0x01,
    LYNX_FRAME_TYPE_HEALTH  = 0x02,
    LYNX_FRAME_TYPE_EVENT   = 0x03,
    LYNX_FRAME_TYPE_COMMAND = 0x04,
    LYNX_FRAME_TYPE_ACK     = 0x05,
} lynx_frame_type_e;

typedef struct {
    uint32_t magic;
    uint8_t  version;
    uint8_t  type;
    uint16_t flags;
    uint32_t sequence;
    uint64_t timestamp_ns;
    uint32_t payload_len;
    uint32_t checksum;
    /* payload follows */
} __attribute__((packed)) lynx_frame_header_t;

typedef struct {
    lynx_frame_header_t header;
    uint8_t *payload;
    size_t payload_capacity;
    int owns_payload;
} lynx_frame_t;

/* Frame initialization and cleanup */
int lynx_frame_init(lynx_frame_t *frame, uint8_t type, uint32_t payload_capacity);
void lynx_frame_free(lynx_frame_t *frame);

/* Encoding: frame -> wire buffer */
int lynx_frame_encode(const lynx_frame_t *frame, uint8_t **out_buf, size_t *out_len);

/* Decoding: wire buffer -> frame */
int lynx_frame_decode(const uint8_t *buf, size_t len, lynx_frame_t *out_frame);

/* Checksum computation */
uint32_t lynx_checksum_crc32(const uint8_t *data, size_t len);

/* Utility: get required buffer size for a frame */
size_t lynx_frame_encoded_size(const lynx_frame_t *frame);

/* Error codes */
#define LYNX_CODEC_OK              0
#define LYNX_CODEC_ERR_NOMEM      -1
#define LYNX_CODEC_ERR_INVAL      -2
#define LYNX_CODEC_ERR_MAGIC      -3
#define LYNX_CODEC_ERR_VERSION     -4
#define LYNX_CODEC_ERR_CHECKSUM   -5
#define LYNX_CODEC_ERR_TRUNCATED  -6
#define LYNX_CODEC_ERR_PAYLOAD    -7

#ifdef __cplusplus
}
#endif

#endif /* LYNX_CODEC_H */