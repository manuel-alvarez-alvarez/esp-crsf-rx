#include "unity.h"

void setUp(void)    {}
void tearDown(void) {}

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

int main(void)
{
    UNITY_BEGIN();

    /* Parser FSM */
    RUN_TEST(test_parser_valid_rc_channels_frame);
    RUN_TEST(test_parser_all_sync_bytes);
    RUN_TEST(test_parser_byte_at_a_time);
    RUN_TEST(test_parser_garbage_before_frame);
    RUN_TEST(test_parser_bad_crc);
    RUN_TEST(test_parser_resync_after_invalid_length);
    RUN_TEST(test_parser_null_callback);
    RUN_TEST(test_parser_two_consecutive_frames);
    RUN_TEST(test_parser_length_too_small);
    RUN_TEST(test_parser_length_too_large);
    RUN_TEST(test_parser_max_payload);
    RUN_TEST(test_parser_feed_empty);
    RUN_TEST(test_parser_resync_sync_as_length);
    RUN_TEST(test_parser_multiple_frames_single_feed);
    RUN_TEST(test_parser_interleaved_garbage);
    RUN_TEST(test_parser_bad_crc_then_valid);
    RUN_TEST(test_parser_non_sync_byte_idle);
    RUN_TEST(test_parser_frame_type_and_payload_passed);

    /* Channel unpacking */
    RUN_TEST(test_channels_all_zeros);
    RUN_TEST(test_channels_all_max);
    RUN_TEST(test_channels_known_midpoint);
    RUN_TEST(test_channels_single_channel_hot);
    RUN_TEST(test_channels_alternating);
    RUN_TEST(test_channels_ascending);

    /* Telemetry unpacking */
    RUN_TEST(test_unpack_link_stats);
    RUN_TEST(test_unpack_link_stats_negative_snr);
    RUN_TEST(test_unpack_gps);
    RUN_TEST(test_unpack_gps_negative_coords);
    RUN_TEST(test_unpack_gps_zero);
    RUN_TEST(test_unpack_vario);
    RUN_TEST(test_unpack_vario_negative);
    RUN_TEST(test_unpack_battery);
    RUN_TEST(test_unpack_battery_zero);
    RUN_TEST(test_unpack_battery_max_capacity);
    RUN_TEST(test_unpack_baro_altitude_short);
    RUN_TEST(test_unpack_baro_altitude_with_vario);
    RUN_TEST(test_unpack_baro_altitude_negative);
    RUN_TEST(test_unpack_heartbeat);
    RUN_TEST(test_unpack_heartbeat_broadcast);
    RUN_TEST(test_unpack_attitude);
    RUN_TEST(test_unpack_attitude_negative);
    RUN_TEST(test_unpack_flight_mode);
    RUN_TEST(test_unpack_flight_mode_truncation);
    RUN_TEST(test_unpack_flight_mode_empty);
    RUN_TEST(test_unpack_device_ping);
    RUN_TEST(test_unpack_device_info);
    RUN_TEST(test_unpack_device_info_truncated);
    RUN_TEST(test_unpack_device_info_short);
    RUN_TEST(test_unpack_device_info_long_name);

    return UNITY_END();
}
