#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CRSF protocol constants */
#define CRSF_BAUDRATE           420000
#define CRSF_MAX_FRAME_SIZE     64
#define CRSF_MAX_PAYLOAD_SIZE   60
#define CRSF_NUM_CHANNELS       16

/* CRSF device addresses (used as sync/destination byte) */
typedef enum {
    CRSF_ADDRESS_BROADCAST   = 0x00,
    CRSF_ADDRESS_FLIGHT_CTRL = 0xC8,
    CRSF_ADDRESS_RADIO       = 0xEE,
    CRSF_ADDRESS_RECEIVER    = 0xEC,
    CRSF_ADDRESS_TX_MODULE   = 0xEA,
} crsf_address_t;

/* CRSF frame types */
typedef enum {
    CRSF_FRAMETYPE_GPS                = 0x02,
    CRSF_FRAMETYPE_VARIO              = 0x07,
    CRSF_FRAMETYPE_BATTERY_SENSOR     = 0x08,
    CRSF_FRAMETYPE_BARO_ALTITUDE      = 0x09,
    CRSF_FRAMETYPE_HEARTBEAT          = 0x0B,
    CRSF_FRAMETYPE_LINK_STATISTICS    = 0x14,
    CRSF_FRAMETYPE_RC_CHANNELS_PACKED = 0x16,
    CRSF_FRAMETYPE_ATTITUDE           = 0x1E,
    CRSF_FRAMETYPE_FLIGHT_MODE        = 0x21,
    CRSF_FRAMETYPE_DEVICE_PING        = 0x28,
    CRSF_FRAMETYPE_DEVICE_INFO        = 0x29,
} crsf_frame_type_t;

/* 16 RC channels, 11-bit values (0-2047) */
typedef struct {
    uint16_t ch[CRSF_NUM_CHANNELS];
} crsf_channels_t;

/* Link statistics */
typedef struct {
    uint8_t uplink_rssi_ant1;       /* dBm * -1 */
    uint8_t uplink_rssi_ant2;       /* dBm * -1 */
    uint8_t uplink_link_quality;    /* % */
    int8_t  uplink_snr;             /* dB */
    uint8_t active_antenna;
    uint8_t rf_mode;
    uint8_t uplink_tx_power;        /* mW */
    uint8_t downlink_rssi;          /* dBm * -1 */
    uint8_t downlink_link_quality;  /* % */
    int8_t  downlink_snr;           /* dB */
} crsf_link_stats_t;

/* GPS — 15 bytes on wire, all big-endian */
typedef struct {
    int32_t  latitude;      /* degrees * 1e7 */
    int32_t  longitude;     /* degrees * 1e7 */
    uint16_t groundspeed;   /* km/h * 10 */
    uint16_t heading;       /* degrees * 100 */
    uint16_t altitude;      /* meters + 1000 offset (0 = -1000m) */
    uint8_t  satellites;
} crsf_gps_t;

/* Variometer — 2 bytes on wire, big-endian */
typedef struct {
    int16_t vertical_speed; /* cm/s */
} crsf_vario_t;

/* Battery sensor — 8 bytes on wire, big-endian */
typedef struct {
    uint16_t voltage;       /* V * 10 */
    uint16_t current;       /* A * 10 */
    uint32_t capacity;      /* mAh (24-bit on wire) */
    uint8_t  remaining;     /* % */
} crsf_battery_t;

/* Barometric altitude — 2 or 4 bytes on wire, big-endian */
typedef struct {
    int16_t altitude;       /* decimeters (wire value − 10000) */
    int16_t vario;          /* cm/s (0 when not present) */
} crsf_baro_altitude_t;

/* Heartbeat — 2 bytes on wire */
typedef struct {
    uint16_t origin;        /* origin device address */
} crsf_heartbeat_t;

/* Attitude — 6 bytes on wire, big-endian */
typedef struct {
    int16_t pitch;          /* radians * 10000 */
    int16_t roll;           /* radians * 10000 */
    int16_t yaw;            /* radians * 10000 */
} crsf_attitude_t;

/* Flight mode — variable length, null-terminated string on wire */
#define CRSF_FLIGHT_MODE_MAX_LEN 16
typedef struct {
    char mode[CRSF_FLIGHT_MODE_MAX_LEN];
} crsf_flight_mode_t;

/* Device ping (extended frame) — destination/origin only, no extra payload */
typedef struct {
    uint8_t destination;
    uint8_t origin;
} crsf_device_ping_t;

/* Device info (extended frame) — variable length */
#define CRSF_DEVICE_NAME_MAX_LEN 48
typedef struct {
    char     name[CRSF_DEVICE_NAME_MAX_LEN]; /* null-terminated */
    uint32_t serial_number;
    uint32_t hardware_id;
    uint32_t firmware_id;
    uint8_t  parameter_count;
    uint8_t  protocol_version;
} crsf_device_info_t;

/* Tagged union of all CRSF frame payloads */
typedef struct {
    crsf_frame_type_t type;
    union {
        crsf_channels_t      channels;
        crsf_link_stats_t    link_stats;
        crsf_gps_t           gps;
        crsf_vario_t         vario;
        crsf_battery_t       battery;
        crsf_baro_altitude_t baro_altitude;
        crsf_heartbeat_t     heartbeat;
        crsf_attitude_t      attitude;
        crsf_flight_mode_t   flight_mode;
        crsf_device_ping_t   device_ping;
        crsf_device_info_t   device_info;
    };
} crsf_message_t;

#ifdef __cplusplus
}
#endif
