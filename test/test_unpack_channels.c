#include "unity.h"
#include "test_helpers.h"

/* ── Tests ───────────────────────────────────────────────────────────── */

void test_channels_all_zeros(void)
{
    uint8_t packed[22];
    memset(packed, 0, sizeof(packed));
    crsf_channels_t ch;
    crsf_unpack_channels(packed, &ch);

    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, ch.ch[i]);
    }
}

void test_channels_all_max(void)
{
    /* All bits set → all 22 bytes = 0xFF → every 11-bit channel = 2047 */
    uint8_t packed[22];
    memset(packed, 0xFF, sizeof(packed));
    crsf_channels_t ch;
    crsf_unpack_channels(packed, &ch);

    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_UINT16(2047, ch.ch[i]);
    }
}

void test_channels_known_midpoint(void)
{
    /* CRSF midpoint: 992 */
    uint16_t expected[CRSF_NUM_CHANNELS];
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) expected[i] = 992;

    uint8_t packed[22];
    pack_channels(expected, packed);

    crsf_channels_t ch;
    crsf_unpack_channels(packed, &ch);

    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_UINT16(992, ch.ch[i]);
    }
}

void test_channels_single_channel_hot(void)
{
    /* Only channel 0 = 2047, rest = 0 */
    uint16_t input[CRSF_NUM_CHANNELS];
    memset(input, 0, sizeof(input));
    input[0] = 2047;

    uint8_t packed[22];
    pack_channels(input, packed);

    crsf_channels_t ch;
    crsf_unpack_channels(packed, &ch);

    TEST_ASSERT_EQUAL_UINT16(2047, ch.ch[0]);
    for (int i = 1; i < CRSF_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, ch.ch[i]);
    }
}

void test_channels_alternating(void)
{
    /* Even channels = 0, odd channels = 2047 */
    uint16_t input[CRSF_NUM_CHANNELS];
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        input[i] = (i % 2 == 0) ? 0 : 2047;
    }

    uint8_t packed[22];
    pack_channels(input, packed);

    crsf_channels_t ch;
    crsf_unpack_channels(packed, &ch);

    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_UINT16(input[i], ch.ch[i]);
    }
}

void test_channels_ascending(void)
{
    /* Each channel = i * 128 (0, 128, 256, ... 1920) */
    uint16_t input[CRSF_NUM_CHANNELS];
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        input[i] = (uint16_t)(i * 128);
    }

    uint8_t packed[22];
    pack_channels(input, packed);

    crsf_channels_t ch;
    crsf_unpack_channels(packed, &ch);

    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_UINT16(input[i], ch.ch[i]);
    }
}
