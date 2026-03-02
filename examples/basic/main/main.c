#include <stdio.h>
#include "crsf_rx.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static const char *TAG = "example";

#define CRSF_RX_PIN  13

static QueueHandle_t channels;
static uint32_t frame_hz;

static void on_crsf_message(const crsf_message_t *msg, void *ctx) {
    static int64_t prev_us;
    static uint32_t avg_dt_us;

    int64_t now = esp_timer_get_time();
    if (prev_us) {
        uint32_t dt = (uint32_t)(now - prev_us);
        if (dt) {
            /* EMA on period: avg = avg + (dt - avg) / 16 */
            avg_dt_us = avg_dt_us ? avg_dt_us + ((int32_t)(dt - avg_dt_us) >> 4) : dt;
            frame_hz = 1000000 / avg_dt_us;
        }
    }
    prev_us = now;

    switch (msg->type) {
        case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
            xQueueOverwrite(channels, &msg->channels);
            break;
        default:
            break;
    }
}

static void print_message(const crsf_channels_t *msg) {
    ESP_LOGI(TAG, "CH 1-8:  %4u %4u %4u %4u %4u %4u %4u %4u",
             msg->ch[0], msg->ch[1],
             msg->ch[2], msg->ch[3],
             msg->ch[4], msg->ch[5],
             msg->ch[6], msg->ch[7]);
    ESP_LOGI(TAG, "CH 9-16: %4u %4u %4u %4u %4u %4u %4u %4u",
             msg->ch[8], msg->ch[9],
             msg->ch[10], msg->ch[11],
             msg->ch[12], msg->ch[13],
             msg->ch[14], msg->ch[15]);
}

void app_main(void) {
    channels = xQueueCreate(1, sizeof(crsf_channels_t));

    crsf_rx_config_t cfg = CRSF_RX_DEFAULT_CONFIG();
    cfg.uart_port = UART_NUM_1;
    cfg.rx_pin = CRSF_RX_PIN;
    cfg.cb = on_crsf_message;

    crsf_rx_handle_t rx;
    ESP_ERROR_CHECK(crsf_rx_init(&cfg, &rx));

    crsf_channels_t msg;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "--- %lu Hz ---", (unsigned long)frame_hz);
        if (xQueueReceive(channels, &msg, 0) == pdTRUE) {
            print_message(&msg);
        }
    }
}
