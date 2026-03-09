#include "crsf_rx.h"
#include "crsf_parser.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "crsf_rx";

#define DEFAULT_PRIORITY            10
#define DEFAULT_STACK               4096
#define DEFAULT_UART_RX_BUF_SIZE    1024
#define DEFAULT_EVT_QUEUE_SIZE      16

struct crsf_rx {
    uart_port_t     uart_port;
    QueueHandle_t   evt_queue;
    TaskHandle_t    task;
    crsf_parser_t   parser;
    volatile bool   running;
    TaskHandle_t    joiner; /* task waiting for rx task to exit */

    crsf_rx_cb_t    cb;
    void           *user_ctx;
};

/* ── Frame dispatch (runs in the receiver task) ────────────────────── */

static void on_frame(const crsf_frame_t *frame, void *ctx)
{
    struct crsf_rx *rx = (struct crsf_rx *)ctx;
    crsf_message_t msg = { .type = (crsf_frame_type_t)frame->type };

    switch (msg.type) {
    case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
        if (frame->payload_len < 22) return;
        crsf_unpack_channels(frame->payload, &msg.channels);
        break;

    case CRSF_FRAMETYPE_LINK_STATISTICS:
        if (frame->payload_len < 10) return;
        crsf_unpack_link_stats(frame->payload, &msg.link_stats);
        break;

    case CRSF_FRAMETYPE_GPS:
        if (frame->payload_len < 15) return;
        crsf_unpack_gps(frame->payload, &msg.gps);
        break;

    case CRSF_FRAMETYPE_VARIO:
        if (frame->payload_len < 2) return;
        crsf_unpack_vario(frame->payload, &msg.vario);
        break;

    case CRSF_FRAMETYPE_BATTERY_SENSOR:
        if (frame->payload_len < 8) return;
        crsf_unpack_battery(frame->payload, &msg.battery);
        break;

    case CRSF_FRAMETYPE_BARO_ALTITUDE:
        if (frame->payload_len < 2) return;
        crsf_unpack_baro_altitude(frame->payload, frame->payload_len, &msg.baro_altitude);
        break;

    case CRSF_FRAMETYPE_HEARTBEAT:
        if (frame->payload_len < 2) return;
        crsf_unpack_heartbeat(frame->payload, &msg.heartbeat);
        break;

    case CRSF_FRAMETYPE_ATTITUDE:
        if (frame->payload_len < 6) return;
        crsf_unpack_attitude(frame->payload, &msg.attitude);
        break;

    case CRSF_FRAMETYPE_FLIGHT_MODE:
        if (frame->payload_len < 1) return;
        crsf_unpack_flight_mode(frame->payload, frame->payload_len, &msg.flight_mode);
        break;

    case CRSF_FRAMETYPE_DEVICE_PING:
        if (frame->payload_len < 2) return;
        crsf_unpack_device_ping(frame->payload, &msg.device_ping);
        break;

    case CRSF_FRAMETYPE_DEVICE_INFO:
        if (frame->payload_len < 2) return;
        crsf_unpack_device_info(frame->payload, frame->payload_len, &msg.device_info);
        break;

    default:
        return;
    }

    if (rx->cb) {
        rx->cb(&msg, rx->user_ctx);
    }
}

/* ── Receiver task ─────────────────────────────────────────────────── */

static void crsf_rx_task(void *arg)
{
    struct crsf_rx *rx = (struct crsf_rx *)arg;
    uint8_t buf[CRSF_MAX_FRAME_SIZE];
    uart_event_t evt;

    while (rx->running) {
        if (!xQueueReceive(rx->evt_queue, &evt, pdMS_TO_TICKS(100))) {
            continue;
        }
        switch (evt.type) {
        case UART_DATA:
            size_t pending = evt.size;
            while (pending > 0) {
                size_t chunk = pending > sizeof(buf) ? sizeof(buf) : pending;
                int n = uart_read_bytes(rx->uart_port, buf, chunk, 0);
                if (n <= 0) {
                    break;
                }
                crsf_parser_feed(&rx->parser, buf, (size_t)n);
                pending -= (size_t)n;
            }
            break;
        case UART_FIFO_OVF:
        case UART_BUFFER_FULL:
        case UART_BREAK:
        case UART_PARITY_ERR:
        case UART_FRAME_ERR:
            ESP_LOGW(TAG, "UART error: type=%d size=%d — flushing input", (int)evt.type, (int)evt.size);
            uart_flush_input(rx->uart_port);
            xQueueReset(rx->evt_queue);
            crsf_parser_reset(&rx->parser);
            break;
        default:
            break;
        }
    }
    /* Signal deinit() that we are done */
    if (rx->joiner) {
        xTaskNotifyGive(rx->joiner);
    }
    vTaskDelete(NULL);
}

/* ── Public API ────────────────────────────────────────────────────── */

esp_err_t crsf_rx_init(const crsf_rx_config_t *config, crsf_rx_handle_t *out_handle)
{
    if (!config || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->rx_pin < 0) {
        ESP_LOGE(TAG, "rx_pin must be a valid GPIO");
        return ESP_ERR_INVALID_ARG;
    }

    struct crsf_rx *rx = calloc(1, sizeof(*rx));
    if (!rx) {
        return ESP_ERR_NO_MEM;
    }

    rx->uart_port = config->uart_port;
    rx->cb        = config->cb;
    rx->user_ctx  = config->user_ctx;

    crsf_parser_init(&rx->parser, on_frame, rx);

    /* UART configuration */
    uint32_t baud = config->baudrate ? config->baudrate : CRSF_BAUDRATE;

    const uart_config_t uart_cfg = {
        .baud_rate  = (int)baud,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;

    const int rx_buf = (config->uart_rx_buf_size > 0)
        ? (int)config->uart_rx_buf_size
        : DEFAULT_UART_RX_BUF_SIZE;
    const int evt_q = (config->uart_evt_queue_size > 0)
        ? config->uart_evt_queue_size
        : DEFAULT_EVT_QUEUE_SIZE;
    err = uart_driver_install(config->uart_port, rx_buf, 0,
                              evt_q, &rx->evt_queue, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        free(rx);
        return err;
    }

    err = uart_param_config(config->uart_port, &uart_cfg);
    if (err != ESP_OK) {
        uart_driver_delete(config->uart_port);
        free(rx);
        return err;
    }

    err = uart_set_pin(config->uart_port, UART_PIN_NO_CHANGE, config->rx_pin,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        uart_driver_delete(config->uart_port);
        free(rx);
        return err;
    }

    /* Optional UART driver tuning knobs (leave defaults if 0) */
    if (config->uart_rx_full_thresh > 0) {
        (void) uart_set_rx_full_threshold(config->uart_port, config->uart_rx_full_thresh);
    }
    if (config->uart_rx_timeout_thresh > 0) {
        (void) uart_set_rx_timeout(config->uart_port, config->uart_rx_timeout_thresh);
    }

    /* Start receiver task */
    rx->running = true;

    int priority      = config->task_priority > 0 ? config->task_priority : DEFAULT_PRIORITY;
    size_t stack      = config->task_stack_size > 0 ? config->task_stack_size : DEFAULT_STACK;
    BaseType_t core   = config->task_core >= 0 ? config->task_core : tskNO_AFFINITY;

    BaseType_t ret = xTaskCreatePinnedToCore(
        crsf_rx_task, "crsf_rx", (uint32_t)stack, rx, priority, &rx->task, core);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "failed to create task");
        uart_driver_delete(config->uart_port);
        free(rx);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "started on UART%d RX:%d @ %lu baud",
             (int)config->uart_port, config->rx_pin, (unsigned long)baud);

    *out_handle = rx;
    return ESP_OK;
}

esp_err_t crsf_rx_deinit(crsf_rx_handle_t *handle)
{
    if (!handle || !*handle) {
        return ESP_ERR_INVALID_ARG;
    }

    struct crsf_rx *rx = *handle;
    TaskHandle_t caller = xTaskGetCurrentTaskHandle();

    if (caller == rx->task) {
        ESP_LOGE(TAG, "crsf_rx_deinit must not be called from the receiver task");
        return ESP_ERR_INVALID_STATE;
    }

    /* Ask the task to stop and wait for it to finish */
    rx->joiner = caller;
    rx->running = false;
    if (rx->evt_queue) {
        xQueueReset(rx->evt_queue);
    }

    const TickType_t join_timeout = pdMS_TO_TICKS(500);
    esp_err_t wait_result = ESP_OK;
    if (rx->task) {
        uint32_t notified = ulTaskNotifyTake(pdTRUE, join_timeout);
        if (notified == 0) {
            ESP_LOGW(TAG, "timeout waiting for receiver task to stop; forcing delete");
            vTaskDelete(rx->task);
            wait_result = ESP_ERR_TIMEOUT;
        }
    }
    rx->task = NULL;
    rx->joiner = NULL;

    uart_driver_delete(rx->uart_port);
    free(rx);
    *handle = NULL;

    ESP_LOGI(TAG, "stopped");
    return wait_result;
}
