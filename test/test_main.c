#include "unity.h"
#include <string.h>

void setUp(void)    {}
void tearDown(void) {}

/* ── CLI filter: -n <test_name> runs only that test ─────────────────── */
static const char *g_filter = NULL;

#define RUN_FILTERED_TEST(func)                              \
    do {                                                     \
        if (!g_filter || strcmp(g_filter, #func) == 0)        \
            RUN_TEST(func);                                  \
    } while (0)

/* ── test_parser_fsm.c ──────────────────────────────────────────────── */
extern void test_parser_valid_rc_channels_frame(void);
extern void test_parser_all_sync_bytes(void);
extern void test_parser_byte_at_a_time(void);
extern void test_parser_garbage_before_frame(void);
extern void test_parser_bad_crc(void);
extern void test_parser_resync_after_invalid_length(void);
extern void test_parser_null_callback(void);
extern void test_parser_two_consecutive_frames(void);
extern void test_parser_length_too_small(void);
extern void test_parser_length_too_large(void);
extern void test_parser_max_payload(void);
extern void test_parser_feed_empty(void);
extern void test_parser_resync_sync_as_length(void);
extern void test_parser_multiple_frames_single_feed(void);
extern void test_parser_interleaved_garbage(void);
extern void test_parser_bad_crc_then_valid(void);
extern void test_parser_non_sync_byte_idle(void);
extern void test_parser_frame_type_and_payload_passed(void);

/* ── test_unpack_channels.c ─────────────────────────────────────────── */
extern void test_channels_all_zeros(void);
extern void test_channels_all_max(void);
extern void test_channels_known_midpoint(void);
extern void test_channels_single_channel_hot(void);
extern void test_channels_alternating(void);
extern void test_channels_ascending(void);

/* ── test_unpack_telemetry.c ────────────────────────────────────────── */
extern void test_unpack_link_stats(void);
extern void test_unpack_link_stats_negative_snr(void);
extern void test_unpack_gps(void);
extern void test_unpack_gps_negative_coords(void);
extern void test_unpack_gps_zero(void);
extern void test_unpack_vario(void);
extern void test_unpack_vario_negative(void);
extern void test_unpack_battery(void);
extern void test_unpack_battery_zero(void);
extern void test_unpack_battery_max_capacity(void);
extern void test_unpack_baro_altitude_short(void);
extern void test_unpack_baro_altitude_with_vario(void);
extern void test_unpack_baro_altitude_negative(void);
extern void test_unpack_heartbeat(void);
extern void test_unpack_heartbeat_broadcast(void);
extern void test_unpack_attitude(void);
extern void test_unpack_attitude_negative(void);
extern void test_unpack_flight_mode(void);
extern void test_unpack_flight_mode_truncation(void);
extern void test_unpack_flight_mode_empty(void);
extern void test_unpack_device_ping(void);
extern void test_unpack_device_info(void);
extern void test_unpack_device_info_truncated(void);
extern void test_unpack_device_info_short(void);
extern void test_unpack_device_info_long_name(void);

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
            g_filter = argv[++i];
    }

    UNITY_BEGIN();

    /* Parser FSM */
    RUN_FILTERED_TEST(test_parser_valid_rc_channels_frame);
    RUN_FILTERED_TEST(test_parser_all_sync_bytes);
    RUN_FILTERED_TEST(test_parser_byte_at_a_time);
    RUN_FILTERED_TEST(test_parser_garbage_before_frame);
    RUN_FILTERED_TEST(test_parser_bad_crc);
    RUN_FILTERED_TEST(test_parser_resync_after_invalid_length);
    RUN_FILTERED_TEST(test_parser_null_callback);
    RUN_FILTERED_TEST(test_parser_two_consecutive_frames);
    RUN_FILTERED_TEST(test_parser_length_too_small);
    RUN_FILTERED_TEST(test_parser_length_too_large);
    RUN_FILTERED_TEST(test_parser_max_payload);
    RUN_FILTERED_TEST(test_parser_feed_empty);
    RUN_FILTERED_TEST(test_parser_resync_sync_as_length);
    RUN_FILTERED_TEST(test_parser_multiple_frames_single_feed);
    RUN_FILTERED_TEST(test_parser_interleaved_garbage);
    RUN_FILTERED_TEST(test_parser_bad_crc_then_valid);
    RUN_FILTERED_TEST(test_parser_non_sync_byte_idle);
    RUN_FILTERED_TEST(test_parser_frame_type_and_payload_passed);

    /* Channel unpacking */
    RUN_FILTERED_TEST(test_channels_all_zeros);
    RUN_FILTERED_TEST(test_channels_all_max);
    RUN_FILTERED_TEST(test_channels_known_midpoint);
    RUN_FILTERED_TEST(test_channels_single_channel_hot);
    RUN_FILTERED_TEST(test_channels_alternating);
    RUN_FILTERED_TEST(test_channels_ascending);

    /* Telemetry unpacking */
    RUN_FILTERED_TEST(test_unpack_link_stats);
    RUN_FILTERED_TEST(test_unpack_link_stats_negative_snr);
    RUN_FILTERED_TEST(test_unpack_gps);
    RUN_FILTERED_TEST(test_unpack_gps_negative_coords);
    RUN_FILTERED_TEST(test_unpack_gps_zero);
    RUN_FILTERED_TEST(test_unpack_vario);
    RUN_FILTERED_TEST(test_unpack_vario_negative);
    RUN_FILTERED_TEST(test_unpack_battery);
    RUN_FILTERED_TEST(test_unpack_battery_zero);
    RUN_FILTERED_TEST(test_unpack_battery_max_capacity);
    RUN_FILTERED_TEST(test_unpack_baro_altitude_short);
    RUN_FILTERED_TEST(test_unpack_baro_altitude_with_vario);
    RUN_FILTERED_TEST(test_unpack_baro_altitude_negative);
    RUN_FILTERED_TEST(test_unpack_heartbeat);
    RUN_FILTERED_TEST(test_unpack_heartbeat_broadcast);
    RUN_FILTERED_TEST(test_unpack_attitude);
    RUN_FILTERED_TEST(test_unpack_attitude_negative);
    RUN_FILTERED_TEST(test_unpack_flight_mode);
    RUN_FILTERED_TEST(test_unpack_flight_mode_truncation);
    RUN_FILTERED_TEST(test_unpack_flight_mode_empty);
    RUN_FILTERED_TEST(test_unpack_device_ping);
    RUN_FILTERED_TEST(test_unpack_device_info);
    RUN_FILTERED_TEST(test_unpack_device_info_truncated);
    RUN_FILTERED_TEST(test_unpack_device_info_short);
    RUN_FILTERED_TEST(test_unpack_device_info_long_name);

    return UNITY_END();
}
