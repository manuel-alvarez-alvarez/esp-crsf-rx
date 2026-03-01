#pragma once

#include <stdint.h>
#include <stddef.h>
#include "crsf_protocol.h"

typedef enum {
    CRSF_PARSE_IDLE,
    CRSF_PARSE_LEN,
    CRSF_PARSE_DATA,
} crsf_parse_state_t;

typedef struct {
    uint8_t type;
    const uint8_t *payload;
    uint8_t payload_len;
} crsf_frame_t;

typedef void (*crsf_parser_frame_cb_t)(const crsf_frame_t *frame, void *ctx);

typedef struct {
    crsf_parse_state_t state;
    uint8_t buf[CRSF_MAX_FRAME_SIZE];
    uint8_t pos;
    uint8_t data_len;
    crsf_parser_frame_cb_t frame_cb;
    void *cb_ctx;
} crsf_parser_t;

void crsf_parser_init(crsf_parser_t *parser, crsf_parser_frame_cb_t cb, void *ctx);
void crsf_parser_feed(crsf_parser_t *parser, const uint8_t *data, size_t len);

void crsf_unpack_channels(const uint8_t *payload, crsf_channels_t *out);
void crsf_unpack_link_stats(const uint8_t *payload, crsf_link_stats_t *out);
void crsf_unpack_gps(const uint8_t *payload, crsf_gps_t *out);
void crsf_unpack_vario(const uint8_t *payload, crsf_vario_t *out);
void crsf_unpack_battery(const uint8_t *payload, crsf_battery_t *out);
void crsf_unpack_baro_altitude(const uint8_t *payload, uint8_t len, crsf_baro_altitude_t *out);
void crsf_unpack_heartbeat(const uint8_t *payload, crsf_heartbeat_t *out);
void crsf_unpack_attitude(const uint8_t *payload, crsf_attitude_t *out);
void crsf_unpack_flight_mode(const uint8_t *payload, uint8_t len, crsf_flight_mode_t *out);
void crsf_unpack_device_ping(const uint8_t *payload, crsf_device_ping_t *out);
void crsf_unpack_device_info(const uint8_t *payload, uint8_t len, crsf_device_info_t *out);
