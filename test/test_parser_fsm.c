#include "unity.h"
#include "test_helpers.h"

/* ── Helpers ─────────────────────────────────────────────────────────── */

static crsf_parser_t parser;
static capture_ctx_t cap;

static void parser_setup(void)
{
    capture_ctx_reset(&cap);
    crsf_parser_init(&parser, capture_cb, &cap);
}

/* ── Tests ───────────────────────────────────────────────────────────── */

void test_parser_valid_rc_channels_frame(void)
{
    parser_setup();
    uint8_t payload[22];
    memset(payload, 0, sizeof(payload));
    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_FLIGHT_CTRL,
                                  CRSF_FRAMETYPE_RC_CHANNELS_PACKED,
                                  payload, 22);
    crsf_parser_feed(&parser, frame, len);

    TEST_ASSERT_EQUAL_INT(1, cap.count);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_RC_CHANNELS_PACKED,
                            cap.frames[0].type);
    TEST_ASSERT_EQUAL_UINT8(22, cap.frames[0].payload_len);
}

void test_parser_all_sync_bytes(void)
{
    const uint8_t syncs[] = {
        CRSF_ADDRESS_FLIGHT_CTRL,   /* 0xC8 */
        CRSF_ADDRESS_RADIO,         /* 0xEE */
        CRSF_ADDRESS_RECEIVER,      /* 0xEC */
        CRSF_ADDRESS_TX_MODULE,     /* 0xEA */
    };
    uint8_t payload[] = {0xAA};

    for (int i = 0; i < 4; i++) {
        parser_setup();
        uint8_t frame[64];
        size_t len = build_crsf_frame(frame, syncs[i],
                                      CRSF_FRAMETYPE_HEARTBEAT, payload, 1);
        crsf_parser_feed(&parser, frame, len);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, cap.count,
                                      "Sync byte not recognized");
    }
}

void test_parser_byte_at_a_time(void)
{
    parser_setup();
    uint8_t payload[] = {0x01, 0x02, 0x03};
    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_FLIGHT_CTRL,
                                  CRSF_FRAMETYPE_LINK_STATISTICS,
                                  payload, sizeof(payload));

    for (size_t i = 0; i < len; i++) {
        crsf_parser_feed(&parser, &frame[i], 1);
    }

    TEST_ASSERT_EQUAL_INT(1, cap.count);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_LINK_STATISTICS,
                            cap.frames[0].type);
}

void test_parser_garbage_before_frame(void)
{
    parser_setup();
    uint8_t garbage[] = {0x00, 0x01, 0x55, 0xFF, 0x42, 0x13, 0x37};
    crsf_parser_feed(&parser, garbage, sizeof(garbage));
    TEST_ASSERT_EQUAL_INT(0, cap.count);

    uint8_t payload[] = {0xBB};
    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_FLIGHT_CTRL,
                                  CRSF_FRAMETYPE_HEARTBEAT, payload, 1);
    crsf_parser_feed(&parser, frame, len);
    TEST_ASSERT_EQUAL_INT(1, cap.count);
}

void test_parser_bad_crc(void)
{
    parser_setup();
    uint8_t payload[] = {0xAA};
    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_FLIGHT_CTRL,
                                  CRSF_FRAMETYPE_HEARTBEAT, payload, 1);
    /* Corrupt the CRC */
    frame[len - 1] ^= 0xFF;
    crsf_parser_feed(&parser, frame, len);

    TEST_ASSERT_EQUAL_INT(0, cap.count);
}

void test_parser_resync_after_invalid_length(void)
{
    parser_setup();
    /* Send sync byte + invalid length (0 and 1 are too small) */
    uint8_t bad[] = {CRSF_ADDRESS_FLIGHT_CTRL, 0x00,
                     CRSF_ADDRESS_FLIGHT_CTRL, 0x01};
    crsf_parser_feed(&parser, bad, sizeof(bad));
    TEST_ASSERT_EQUAL_INT(0, cap.count);

    /* Now send a valid frame; parser should be in LEN state after last sync */
    uint8_t payload[] = {0xCC};
    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_FLIGHT_CTRL,
                                  CRSF_FRAMETYPE_HEARTBEAT, payload, 1);
    crsf_parser_feed(&parser, frame, len);
    TEST_ASSERT_EQUAL_INT(1, cap.count);
}

void test_parser_null_callback(void)
{
    crsf_parser_t p;
    crsf_parser_init(&p, NULL, NULL);

    uint8_t payload[] = {0xAA};
    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_FLIGHT_CTRL,
                                  CRSF_FRAMETYPE_HEARTBEAT, payload, 1);
    /* Should not crash */
    crsf_parser_feed(&p, frame, len);
}

void test_parser_two_consecutive_frames(void)
{
    parser_setup();
    uint8_t payload1[] = {0x01};
    uint8_t payload2[] = {0x02, 0x03};
    uint8_t buf[64];
    size_t off = 0;

    off += build_crsf_frame(&buf[off], CRSF_ADDRESS_FLIGHT_CTRL,
                            CRSF_FRAMETYPE_HEARTBEAT, payload1, 1);
    off += build_crsf_frame(&buf[off], CRSF_ADDRESS_RADIO,
                            CRSF_FRAMETYPE_VARIO, payload2, 2);

    crsf_parser_feed(&parser, buf, off);

    TEST_ASSERT_EQUAL_INT(2, cap.count);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_HEARTBEAT, cap.frames[0].type);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_VARIO, cap.frames[1].type);
}

void test_parser_length_too_small(void)
{
    parser_setup();
    /* Length byte = 1 (invalid, minimum is 2) */
    uint8_t bad[] = {CRSF_ADDRESS_FLIGHT_CTRL, 0x01, 0x00};
    crsf_parser_feed(&parser, bad, sizeof(bad));
    TEST_ASSERT_EQUAL_INT(0, cap.count);
}

void test_parser_length_too_large(void)
{
    parser_setup();
    /* Length byte = 63 (invalid, maximum is 62) */
    uint8_t bad[] = {CRSF_ADDRESS_FLIGHT_CTRL, 63, 0x00};
    crsf_parser_feed(&parser, bad, sizeof(bad));
    TEST_ASSERT_EQUAL_INT(0, cap.count);
}

void test_parser_max_payload(void)
{
    parser_setup();
    /* Maximum valid length = 62 → payload_len = 60 */
    uint8_t payload[60];
    for (int i = 0; i < 60; i++) payload[i] = (uint8_t)i;

    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_FLIGHT_CTRL,
                                  CRSF_FRAMETYPE_RC_CHANNELS_PACKED,
                                  payload, 60);
    crsf_parser_feed(&parser, frame, len);

    TEST_ASSERT_EQUAL_INT(1, cap.count);
    TEST_ASSERT_EQUAL_UINT8(60, cap.frames[0].payload_len);
}

void test_parser_feed_empty(void)
{
    parser_setup();
    uint8_t dummy = 0;
    crsf_parser_feed(&parser, &dummy, 0);
    TEST_ASSERT_EQUAL_INT(0, cap.count);
}

void test_parser_resync_sync_as_length(void)
{
    parser_setup();
    /*
     * First byte: 0xC8 (sync) → IDLE→LEN
     * Second byte: 0xC8 (200 > 62, invalid length, but IS a sync byte)
     * Parser should resync: treat 0xC8 as new sync byte.
     * Third byte onward: valid frame using 0xC8 as sync.
     */
    uint8_t payload[] = {0xDD};
    uint8_t frame[64];
    /* Build valid frame starting at offset 1 */
    size_t flen = build_crsf_frame(frame + 1, CRSF_ADDRESS_FLIGHT_CTRL,
                                   CRSF_FRAMETYPE_HEARTBEAT, payload, 1);
    /* Prepend an extra sync byte that becomes "bad" sync */
    frame[0] = CRSF_ADDRESS_FLIGHT_CTRL;
    /* Feed: [0xC8] [0xC8 (bad len → resync)] [len] [type] [payload] [crc] */
    crsf_parser_feed(&parser, frame, 1 + flen);

    TEST_ASSERT_EQUAL_INT(1, cap.count);
}

void test_parser_multiple_frames_single_feed(void)
{
    parser_setup();
    uint8_t buf[128];
    size_t off = 0;

    for (int i = 0; i < 5; i++) {
        uint8_t payload[] = {(uint8_t)i};
        off += build_crsf_frame(&buf[off], CRSF_ADDRESS_FLIGHT_CTRL,
                                CRSF_FRAMETYPE_HEARTBEAT, payload, 1);
    }
    crsf_parser_feed(&parser, buf, off);

    TEST_ASSERT_EQUAL_INT(5, cap.count);
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_UINT8((uint8_t)i, cap.payloads[i][0]);
    }
}

void test_parser_interleaved_garbage(void)
{
    parser_setup();
    uint8_t buf[128];
    size_t off = 0;

    /* Garbage */
    uint8_t garbage[] = {0x00, 0x55, 0xAA, 0x12};
    memcpy(&buf[off], garbage, sizeof(garbage));
    off += sizeof(garbage);

    /* Frame 1 */
    uint8_t p1[] = {0x11};
    off += build_crsf_frame(&buf[off], CRSF_ADDRESS_FLIGHT_CTRL,
                            CRSF_FRAMETYPE_HEARTBEAT, p1, 1);
    /* More garbage */
    uint8_t garbage2[] = {0x33, 0x44};
    memcpy(&buf[off], garbage2, sizeof(garbage2));
    off += sizeof(garbage2);

    /* Frame 2 */
    uint8_t p2[] = {0x22};
    off += build_crsf_frame(&buf[off], CRSF_ADDRESS_RADIO,
                            CRSF_FRAMETYPE_VARIO, p2, 1);

    crsf_parser_feed(&parser, buf, off);

    TEST_ASSERT_EQUAL_INT(2, cap.count);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_HEARTBEAT, cap.frames[0].type);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_VARIO, cap.frames[1].type);
}

void test_parser_bad_crc_then_valid(void)
{
    parser_setup();
    uint8_t buf[64];
    size_t off = 0;

    /* Bad-CRC frame */
    uint8_t p1[] = {0xAA};
    off += build_crsf_frame(&buf[off], CRSF_ADDRESS_FLIGHT_CTRL,
                            CRSF_FRAMETYPE_HEARTBEAT, p1, 1);
    buf[off - 1] ^= 0xFF; /* corrupt CRC */

    /* Valid frame */
    uint8_t p2[] = {0xBB};
    off += build_crsf_frame(&buf[off], CRSF_ADDRESS_FLIGHT_CTRL,
                            CRSF_FRAMETYPE_VARIO, p2, 1);

    crsf_parser_feed(&parser, buf, off);

    TEST_ASSERT_EQUAL_INT(1, cap.count);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_VARIO, cap.frames[0].type);
}

void test_parser_non_sync_byte_idle(void)
{
    parser_setup();
    /* All non-sync bytes — none should trigger state change */
    uint8_t data[] = {0x00, 0x01, 0x42, 0x99, 0xAB, 0xFF};
    crsf_parser_feed(&parser, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(0, cap.count);
    TEST_ASSERT_EQUAL(CRSF_PARSE_IDLE, parser.state);
}

void test_parser_frame_type_and_payload_passed(void)
{
    parser_setup();
    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t frame[64];
    size_t len = build_crsf_frame(frame, CRSF_ADDRESS_RECEIVER,
                                  CRSF_FRAMETYPE_GPS, payload, 4);
    crsf_parser_feed(&parser, frame, len);

    TEST_ASSERT_EQUAL_INT(1, cap.count);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_GPS, cap.frames[0].type);
    TEST_ASSERT_EQUAL_UINT8(4, cap.frames[0].payload_len);
    TEST_ASSERT_EQUAL_UINT8(0xDE, cap.payloads[0][0]);
    TEST_ASSERT_EQUAL_UINT8(0xAD, cap.payloads[0][1]);
    TEST_ASSERT_EQUAL_UINT8(0xBE, cap.payloads[0][2]);
    TEST_ASSERT_EQUAL_UINT8(0xEF, cap.payloads[0][3]);
}
