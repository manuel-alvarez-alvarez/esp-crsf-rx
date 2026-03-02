#include "unity.h"
#include "test_helpers.h"

/* ── Link Statistics ─────────────────────────────────────────────────── */

void test_unpack_link_stats(void)
{
    uint8_t p[] = {100, 95, 80, 5, 1, 4, 25, 110, 70, 3};
    crsf_link_stats_t s;
    crsf_unpack_link_stats(p, &s);

    TEST_ASSERT_EQUAL_UINT8(100, s.uplink_rssi_ant1);
    TEST_ASSERT_EQUAL_UINT8(95,  s.uplink_rssi_ant2);
    TEST_ASSERT_EQUAL_UINT8(80,  s.uplink_link_quality);
    TEST_ASSERT_EQUAL_INT8(5,    s.uplink_snr);
    TEST_ASSERT_EQUAL_UINT8(1,   s.active_antenna);
    TEST_ASSERT_EQUAL_UINT8(4,   s.rf_mode);
    TEST_ASSERT_EQUAL_UINT8(25,  s.uplink_tx_power);
    TEST_ASSERT_EQUAL_UINT8(110, s.downlink_rssi);
    TEST_ASSERT_EQUAL_UINT8(70,  s.downlink_link_quality);
    TEST_ASSERT_EQUAL_INT8(3,    s.downlink_snr);
}

void test_unpack_link_stats_negative_snr(void)
{
    /* SNR fields: -10 = 0xF6, -20 = 0xEC */
    uint8_t p[] = {50, 60, 99, 0xF6, 0, 2, 50, 80, 90, 0xEC};
    crsf_link_stats_t s;
    crsf_unpack_link_stats(p, &s);

    TEST_ASSERT_EQUAL_INT8(-10, s.uplink_snr);
    TEST_ASSERT_EQUAL_INT8(-20, s.downlink_snr);
}

/* ── GPS ─────────────────────────────────────────────────────────────── */

void test_unpack_gps(void)
{
    /* lat=471234567 (0x1C167807), lon=82345678 (0x04E87ECE),
       speed=256, heading=512, alt=768, sats=12 */
    uint8_t p[] = {
        0x1C, 0x16, 0x78, 0x07,   /* latitude (BE) */
        0x04, 0xE8, 0x7E, 0xCE,   /* longitude (BE) */
        0x01, 0x00,                /* groundspeed (BE) */
        0x02, 0x00,                /* heading (BE) */
        0x03, 0x00,                /* altitude (BE) */
        0x0C,                      /* satellites */
    };
    crsf_gps_t g;
    crsf_unpack_gps(p, &g);

    TEST_ASSERT_EQUAL_INT32(471234567, g.latitude);
    TEST_ASSERT_EQUAL_INT32(82345678,  g.longitude);
    TEST_ASSERT_EQUAL_UINT16(256,  g.groundspeed);
    TEST_ASSERT_EQUAL_UINT16(512,  g.heading);
    TEST_ASSERT_EQUAL_UINT16(768,  g.altitude);
    TEST_ASSERT_EQUAL_UINT8(12,    g.satellites);
}

void test_unpack_gps_negative_coords(void)
{
    /* lat=-1 (0xFFFFFFFF), lon=-2 (0xFFFFFFFE) */
    uint8_t p[] = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00,
    };
    crsf_gps_t g;
    crsf_unpack_gps(p, &g);

    TEST_ASSERT_EQUAL_INT32(-1, g.latitude);
    TEST_ASSERT_EQUAL_INT32(-2, g.longitude);
}

void test_unpack_gps_zero(void)
{
    uint8_t p[15];
    memset(p, 0, sizeof(p));
    crsf_gps_t g;
    crsf_unpack_gps(p, &g);

    TEST_ASSERT_EQUAL_INT32(0,  g.latitude);
    TEST_ASSERT_EQUAL_INT32(0,  g.longitude);
    TEST_ASSERT_EQUAL_UINT16(0, g.groundspeed);
    TEST_ASSERT_EQUAL_UINT16(0, g.heading);
    TEST_ASSERT_EQUAL_UINT16(0, g.altitude);
    TEST_ASSERT_EQUAL_UINT8(0,  g.satellites);
}

/* ── Vario ───────────────────────────────────────────────────────────── */

void test_unpack_vario(void)
{
    /* 350 cm/s = 0x015E (BE) */
    uint8_t p[] = {0x01, 0x5E};
    crsf_vario_t v;
    crsf_unpack_vario(p, &v);

    TEST_ASSERT_EQUAL_INT16(350, v.vertical_speed);
}

void test_unpack_vario_negative(void)
{
    /* -200 cm/s = 0xFF38 (BE) */
    uint8_t p[] = {0xFF, 0x38};
    crsf_vario_t v;
    crsf_unpack_vario(p, &v);

    TEST_ASSERT_EQUAL_INT16(-200, v.vertical_speed);
}

/* ── Battery ─────────────────────────────────────────────────────────── */

void test_unpack_battery(void)
{
    /* voltage=168 (0x00A8), current=125 (0x007D),
       capacity=1500 (0x0005DC 24-bit), remaining=75 */
    uint8_t p[] = {0x00, 0xA8, 0x00, 0x7D, 0x00, 0x05, 0xDC, 0x4B};
    crsf_battery_t b;
    crsf_unpack_battery(p, &b);

    TEST_ASSERT_EQUAL_UINT16(168,  b.voltage);
    TEST_ASSERT_EQUAL_UINT16(125,  b.current);
    TEST_ASSERT_EQUAL_UINT32(1500, b.capacity);
    TEST_ASSERT_EQUAL_UINT8(75,    b.remaining);
}

void test_unpack_battery_zero(void)
{
    uint8_t p[8];
    memset(p, 0, sizeof(p));
    crsf_battery_t b;
    crsf_unpack_battery(p, &b);

    TEST_ASSERT_EQUAL_UINT16(0, b.voltage);
    TEST_ASSERT_EQUAL_UINT16(0, b.current);
    TEST_ASSERT_EQUAL_UINT32(0, b.capacity);
    TEST_ASSERT_EQUAL_UINT8(0,  b.remaining);
}

void test_unpack_battery_max_capacity(void)
{
    /* capacity = 0xFFFFFF (24-bit max = 16777215 mAh) */
    uint8_t p[] = {0x00, 0x01, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0x64};
    crsf_battery_t b;
    crsf_unpack_battery(p, &b);

    TEST_ASSERT_EQUAL_UINT16(1, b.voltage);
    TEST_ASSERT_EQUAL_UINT16(2, b.current);
    TEST_ASSERT_EQUAL_UINT32(16777215, b.capacity);
    TEST_ASSERT_EQUAL_UINT8(100, b.remaining);
}

/* ── Barometric Altitude ─────────────────────────────────────────────── */

void test_unpack_baro_altitude_short(void)
{
    /* Wire value = 10100 (0x2774) → altitude = 10100 - 10000 = 100 dm */
    uint8_t p[] = {0x27, 0x74};
    crsf_baro_altitude_t b;
    crsf_unpack_baro_altitude(p, 2, &b);

    TEST_ASSERT_EQUAL_INT16(100, b.altitude);
    TEST_ASSERT_EQUAL_INT16(0,   b.vario);
}

void test_unpack_baro_altitude_with_vario(void)
{
    /* Wire altitude = 10500 (0x2904) → 500 dm, vario = 150 (0x0096) */
    uint8_t p[] = {0x29, 0x04, 0x00, 0x96};
    crsf_baro_altitude_t b;
    crsf_unpack_baro_altitude(p, 4, &b);

    TEST_ASSERT_EQUAL_INT16(500, b.altitude);
    TEST_ASSERT_EQUAL_INT16(150, b.vario);
}

void test_unpack_baro_altitude_negative(void)
{
    /* Wire value = 9800 (0x2648) → 9800 - 10000 = -200 dm */
    uint8_t p[] = {0x26, 0x48};
    crsf_baro_altitude_t b;
    crsf_unpack_baro_altitude(p, 2, &b);

    TEST_ASSERT_EQUAL_INT16(-200, b.altitude);
    TEST_ASSERT_EQUAL_INT16(0,    b.vario);
}

/* ── Heartbeat ───────────────────────────────────────────────────────── */

void test_unpack_heartbeat(void)
{
    /* Origin = 0xC8EE (BE) */
    uint8_t p[] = {0xC8, 0xEE};
    crsf_heartbeat_t h;
    crsf_unpack_heartbeat(p, &h);

    TEST_ASSERT_EQUAL_UINT16(0xC8EE, h.origin);
}

void test_unpack_heartbeat_broadcast(void)
{
    uint8_t p[] = {0x00, 0x00};
    crsf_heartbeat_t h;
    crsf_unpack_heartbeat(p, &h);

    TEST_ASSERT_EQUAL_UINT16(0, h.origin);
}

/* ── Attitude ────────────────────────────────────────────────────────── */

void test_unpack_attitude(void)
{
    /* pitch=1500 (0x05DC), roll=-3000 (0xF448), yaw=7854 (0x1EAE) */
    uint8_t p[] = {0x05, 0xDC, 0xF4, 0x48, 0x1E, 0xAE};
    crsf_attitude_t a;
    crsf_unpack_attitude(p, &a);

    TEST_ASSERT_EQUAL_INT16(1500,  a.pitch);
    TEST_ASSERT_EQUAL_INT16(-3000, a.roll);
    TEST_ASSERT_EQUAL_INT16(7854,  a.yaw);
}

void test_unpack_attitude_negative(void)
{
    /* pitch=-5000 (0xEC78), roll=-10000 (0xD8F0), yaw=-15000 (0xC568) */
    uint8_t p[] = {0xEC, 0x78, 0xD8, 0xF0, 0xC5, 0x68};
    crsf_attitude_t a;
    crsf_unpack_attitude(p, &a);

    TEST_ASSERT_EQUAL_INT16(-5000,  a.pitch);
    TEST_ASSERT_EQUAL_INT16(-10000, a.roll);
    TEST_ASSERT_EQUAL_INT16(-15000, a.yaw);
}

/* ── Flight Mode ─────────────────────────────────────────────────────── */

void test_unpack_flight_mode(void)
{
    uint8_t p[] = {'A', 'C', 'R', 'O'};
    crsf_flight_mode_t f;
    crsf_unpack_flight_mode(p, 4, &f);

    TEST_ASSERT_EQUAL_STRING("ACRO", f.mode);
}

void test_unpack_flight_mode_truncation(void)
{
    /* 16 chars — should be truncated to 15 + null */
    uint8_t p[] = "STABILIZE_MODE_X";
    crsf_flight_mode_t f;
    crsf_unpack_flight_mode(p, 16, &f);

    TEST_ASSERT_EQUAL_size_t(CRSF_FLIGHT_MODE_MAX_LEN - 1, strlen(f.mode));
    TEST_ASSERT_EQUAL_STRING("STABILIZE_MODE_", f.mode);
}

void test_unpack_flight_mode_empty(void)
{
    crsf_flight_mode_t f;
    memset(&f, 0xFF, sizeof(f)); /* fill with junk first */
    crsf_unpack_flight_mode(NULL, 0, &f);

    TEST_ASSERT_EQUAL_STRING("", f.mode);
}

/* ── Device Ping ─────────────────────────────────────────────────────── */

void test_unpack_device_ping(void)
{
    uint8_t p[] = {0xC8, 0xEA};
    crsf_device_ping_t dp;
    crsf_unpack_device_ping(p, &dp);

    TEST_ASSERT_EQUAL_UINT8(0xC8, dp.destination);
    TEST_ASSERT_EQUAL_UINT8(0xEA, dp.origin);
}

/* ── Device Info ─────────────────────────────────────────────────────── */

void test_unpack_device_info(void)
{
    /* dest=0xC8, origin=0xEA, name="ELRS RX\0", then 14 bytes of fields */
    uint8_t p[] = {
        0xC8, 0xEA,                                     /* dest, origin */
        'E', 'L', 'R', 'S', ' ', 'R', 'X', 0x00,       /* name + null */
        0x12, 0x34, 0x56, 0x78,                         /* serial (BE) */
        0xAA, 0xBB, 0xCC, 0xDD,                         /* hw_id (BE) */
        0x00, 0x01, 0x02, 0x03,                         /* fw_id (BE) */
        0x05,                                           /* param_count */
        0x01,                                           /* proto_version */
    };
    crsf_device_info_t di;
    crsf_unpack_device_info(p, sizeof(p), &di);

    TEST_ASSERT_EQUAL_STRING("ELRS RX", di.name);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, di.serial_number);
    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDD, di.hardware_id);
    TEST_ASSERT_EQUAL_UINT32(0x00010203, di.firmware_id);
    TEST_ASSERT_EQUAL_UINT8(5, di.parameter_count);
    TEST_ASSERT_EQUAL_UINT8(1, di.protocol_version);
}

void test_unpack_device_info_truncated(void)
{
    /* Name present but not enough remaining bytes for fields → early return */
    uint8_t p[] = {0xC8, 0xEA, 'A', 'B', 0x00, 0x01};
    crsf_device_info_t di;
    crsf_unpack_device_info(p, sizeof(p), &di);

    TEST_ASSERT_EQUAL_STRING("AB", di.name);
    TEST_ASSERT_EQUAL_UINT32(0, di.serial_number);
    TEST_ASSERT_EQUAL_UINT32(0, di.hardware_id);
    TEST_ASSERT_EQUAL_UINT32(0, di.firmware_id);
    TEST_ASSERT_EQUAL_UINT8(0,  di.parameter_count);
    TEST_ASSERT_EQUAL_UINT8(0,  di.protocol_version);
}

void test_unpack_device_info_short(void)
{
    /* Less than 2 bytes → all zero */
    uint8_t p[] = {0xC8};
    crsf_device_info_t di;
    memset(&di, 0xFF, sizeof(di));
    crsf_unpack_device_info(p, 1, &di);

    TEST_ASSERT_EQUAL_STRING("", di.name);
    TEST_ASSERT_EQUAL_UINT32(0, di.serial_number);
}

void test_unpack_device_info_long_name(void)
{
    /* Name of 50 chars → truncated to CRSF_DEVICE_NAME_MAX_LEN - 1 = 47 */
    uint8_t p[2 + 51 + 14]; /* dest/origin + name(50)+null + 14 fields */
    memset(p, 0, sizeof(p));
    p[0] = 0xC8;
    p[1] = 0xEA;
    memset(&p[2], 'A', 50);
    p[52] = 0x00; /* null terminator after 50 chars */
    /* Fields at p[53] .. p[66] */
    p[53] = 0x00; p[54] = 0x00; p[55] = 0x00; p[56] = 0x01; /* serial */
    p[57] = 0x00; p[58] = 0x00; p[59] = 0x00; p[60] = 0x02; /* hw_id */
    p[61] = 0x00; p[62] = 0x00; p[63] = 0x00; p[64] = 0x03; /* fw_id */
    p[65] = 0x0A; /* param_count */
    p[66] = 0x02; /* proto_version */

    crsf_device_info_t di;
    crsf_unpack_device_info(p, sizeof(p), &di);

    TEST_ASSERT_EQUAL_size_t(CRSF_DEVICE_NAME_MAX_LEN - 1, strlen(di.name));
    TEST_ASSERT_EQUAL_UINT32(1,  di.serial_number);
    TEST_ASSERT_EQUAL_UINT32(2,  di.hardware_id);
    TEST_ASSERT_EQUAL_UINT32(3,  di.firmware_id);
    TEST_ASSERT_EQUAL_UINT8(10,  di.parameter_count);
    TEST_ASSERT_EQUAL_UINT8(2,   di.protocol_version);
}
