#include "crsf_parser.h"
#include <stdbool.h>
#include <string.h>

#include "esp_attr.h"

/* Big-endian readers */
static inline uint16_t rd_be16(const uint8_t *p)  { return (uint16_t)p[0] << 8 | p[1]; }
static inline int16_t  rd_be16s(const uint8_t *p) { return (int16_t)rd_be16(p); }
static inline uint32_t rd_be32(const uint8_t *p)  { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static inline int32_t  rd_be32s(const uint8_t *p) { return (int32_t)rd_be32(p); }
static inline uint32_t rd_be24(const uint8_t *p)  { return (uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2]; }

/* CRC8 DVB-S2 lookup table (polynomial 0xD5) */
static DRAM_ATTR const uint8_t crc8_lut[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54,
    0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06,
    0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0,
    0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2,
    0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9,
    0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B,
    0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D,
    0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F,
    0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB,
    0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9,
    0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F,
    0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D,
    0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26,
    0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74,
    0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82,
    0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0,
    0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9,
};

static uint8_t crc8_calc(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = crc8_lut[crc ^ data[i]];
    }
    return crc;
}

static inline bool is_sync_byte(uint8_t b)
{
    return b == CRSF_ADDRESS_FLIGHT_CTRL
        || b == CRSF_ADDRESS_RADIO
        || b == CRSF_ADDRESS_RECEIVER
        || b == CRSF_ADDRESS_TX_MODULE;
}

void crsf_parser_init(crsf_parser_t *parser, crsf_parser_frame_cb_t cb, void *ctx)
{
    memset(parser, 0, sizeof(*parser));
    parser->frame_cb = cb;
    parser->cb_ctx = ctx;
}

void crsf_parser_reset(crsf_parser_t *parser)
{
    parser->state = CRSF_PARSE_IDLE;
    parser->pos = 0;
}

void crsf_parser_feed(crsf_parser_t *parser, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];

        switch (parser->state) {
        case CRSF_PARSE_IDLE:
            if (is_sync_byte(b)) {
                parser->buf[0] = b;
                parser->pos = 1;
                parser->state = CRSF_PARSE_LEN;
            }
            break;

        case CRSF_PARSE_LEN:
            /* length must cover at least type + crc, and fit in max frame */
            if (b >= 2 && b <= CRSF_MAX_FRAME_SIZE - 2) {
                parser->buf[1] = b;
                parser->data_len = b;
                parser->pos = 2;
                parser->state = CRSF_PARSE_DATA;
            } else {
                /* invalid length — resync */
                parser->state = CRSF_PARSE_IDLE;
                if (is_sync_byte(b)) {
                    parser->buf[0] = b;
                    parser->pos = 1;
                    parser->state = CRSF_PARSE_LEN;
                }
            }
            break;

        case CRSF_PARSE_DATA:
            parser->buf[parser->pos++] = b;

            if (parser->pos == (uint8_t)(parser->data_len + 2)) {
                /* Frame complete: buf layout is [addr][len][type][payload...][crc] */
                uint8_t crc_len = parser->data_len - 1; /* type + payload, excl crc */
                uint8_t expected = crc8_calc(&parser->buf[2], crc_len);
                uint8_t received = parser->buf[parser->pos - 1];

                if (expected == received && parser->frame_cb) {
                    crsf_frame_t frame = {
                        .type        = parser->buf[2],
                        .payload     = &parser->buf[3],
                        .payload_len = parser->data_len - 2, /* excl type and crc */
                    };
                    parser->frame_cb(&frame, parser->cb_ctx);
                }
                parser->state = CRSF_PARSE_IDLE;
            }
            break;
        }
    }
}

/*
 * Unpack 16 channels from 22-byte CRSF payload.
 * Channels are packed as 11-bit values in LSB-first order.
 */
void crsf_unpack_channels(const uint8_t *p, crsf_channels_t *out)
{
    out->ch[0]  = ((uint16_t)p[0]        | (uint16_t)p[1]  << 8)               & 0x07FF;
    out->ch[1]  = ((uint16_t)p[1]  >> 3  | (uint16_t)p[2]  << 5)               & 0x07FF;
    out->ch[2]  = ((uint16_t)p[2]  >> 6  | (uint16_t)p[3]  << 2 | (uint16_t)p[4]  << 10) & 0x07FF;
    out->ch[3]  = ((uint16_t)p[4]  >> 1  | (uint16_t)p[5]  << 7)               & 0x07FF;
    out->ch[4]  = ((uint16_t)p[5]  >> 4  | (uint16_t)p[6]  << 4)               & 0x07FF;
    out->ch[5]  = ((uint16_t)p[6]  >> 7  | (uint16_t)p[7]  << 1 | (uint16_t)p[8]  << 9)  & 0x07FF;
    out->ch[6]  = ((uint16_t)p[8]  >> 2  | (uint16_t)p[9]  << 6)               & 0x07FF;
    out->ch[7]  = ((uint16_t)p[9]  >> 5  | (uint16_t)p[10] << 3)               & 0x07FF;
    out->ch[8]  = ((uint16_t)p[11]       | (uint16_t)p[12] << 8)               & 0x07FF;
    out->ch[9]  = ((uint16_t)p[12] >> 3  | (uint16_t)p[13] << 5)               & 0x07FF;
    out->ch[10] = ((uint16_t)p[13] >> 6  | (uint16_t)p[14] << 2 | (uint16_t)p[15] << 10) & 0x07FF;
    out->ch[11] = ((uint16_t)p[15] >> 1  | (uint16_t)p[16] << 7)               & 0x07FF;
    out->ch[12] = ((uint16_t)p[16] >> 4  | (uint16_t)p[17] << 4)               & 0x07FF;
    out->ch[13] = ((uint16_t)p[17] >> 7  | (uint16_t)p[18] << 1 | (uint16_t)p[19] << 9)  & 0x07FF;
    out->ch[14] = ((uint16_t)p[19] >> 2  | (uint16_t)p[20] << 6)               & 0x07FF;
    out->ch[15] = ((uint16_t)p[20] >> 5  | (uint16_t)p[21] << 3)               & 0x07FF;
}

void crsf_unpack_link_stats(const uint8_t *p, crsf_link_stats_t *out)
{
    out->uplink_rssi_ant1      = p[0];
    out->uplink_rssi_ant2      = p[1];
    out->uplink_link_quality   = p[2];
    out->uplink_snr            = (int8_t)p[3];
    out->active_antenna        = p[4];
    out->rf_mode               = p[5];
    out->uplink_tx_power       = p[6];
    out->downlink_rssi         = p[7];
    out->downlink_link_quality = p[8];
    out->downlink_snr          = (int8_t)p[9];
}

void crsf_unpack_gps(const uint8_t *p, crsf_gps_t *out)
{
    out->latitude    = rd_be32s(p);
    out->longitude   = rd_be32s(p + 4);
    out->groundspeed = rd_be16(p + 8);
    out->heading     = rd_be16(p + 10);
    out->altitude    = rd_be16(p + 12);
    out->satellites  = p[14];
}

void crsf_unpack_vario(const uint8_t *p, crsf_vario_t *out)
{
    out->vertical_speed = rd_be16s(p);
}

void crsf_unpack_battery(const uint8_t *p, crsf_battery_t *out)
{
    out->voltage   = rd_be16(p);
    out->current   = rd_be16(p + 2);
    out->capacity  = rd_be24(p + 4);
    out->remaining = p[7];
}

void crsf_unpack_baro_altitude(const uint8_t *p, uint8_t len, crsf_baro_altitude_t *out)
{
    out->altitude = (int16_t)(rd_be16(p) - 10000);
    out->vario    = (len >= 4) ? rd_be16s(p + 2) : 0;
}

void crsf_unpack_heartbeat(const uint8_t *p, crsf_heartbeat_t *out)
{
    out->origin = rd_be16(p);
}

void crsf_unpack_attitude(const uint8_t *p, crsf_attitude_t *out)
{
    out->pitch = rd_be16s(p);
    out->roll  = rd_be16s(p + 2);
    out->yaw   = rd_be16s(p + 4);
}

void crsf_unpack_flight_mode(const uint8_t *p, uint8_t len, crsf_flight_mode_t *out)
{
    size_t copy = len < CRSF_FLIGHT_MODE_MAX_LEN - 1 ? len : CRSF_FLIGHT_MODE_MAX_LEN - 1;
    memcpy(out->mode, p, copy);
    out->mode[copy] = '\0';
}

void crsf_unpack_device_ping(const uint8_t *p, crsf_device_ping_t *out)
{
    out->destination = p[0];
    out->origin      = p[1];
}

void crsf_unpack_device_info(const uint8_t *p, uint8_t len, crsf_device_info_t *out)
{
    memset(out, 0, sizeof(*out));

    /* Extended frame: first two bytes are destination and origin */
    if (len < 2) {
        return;
    }
    const uint8_t *name_start = p + 2;
    uint8_t remaining = len - 2;

    /* Copy null-terminated device name */
    size_t name_len = 0;
    while (name_len < remaining && name_start[name_len] != '\0') {
        name_len++;
    }
    size_t copy = name_len < CRSF_DEVICE_NAME_MAX_LEN - 1 ? name_len : CRSF_DEVICE_NAME_MAX_LEN - 1;
    memcpy(out->name, name_start, copy);
    out->name[copy] = '\0';

    /* Skip past name + null terminator */
    size_t offset = name_len + 1;
    if (offset + 14 > remaining) {
        return;
    }
    const uint8_t *fields = name_start + offset;
    out->serial_number   = rd_be32(fields);
    out->hardware_id     = rd_be32(fields + 4);
    out->firmware_id     = rd_be32(fields + 8);
    out->parameter_count = fields[12];
    out->protocol_version = fields[13];
}
