#pragma once

#include <esp_err.h>
#include <driver/uart.h>
#include "crsf_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct crsf_rx *crsf_rx_handle_t;

/**
 * Single callback for all decoded CRSF frames.
 * Switch on msg->type, then access the corresponding union member.
 * Called from the receiver task — keep processing minimal (up to 1000Hz).
 */
typedef void (*crsf_rx_cb_t)(const crsf_message_t *msg, void *user_ctx);

typedef struct {
    uart_port_t uart_port;      /* UART peripheral (default: UART_NUM_1) */
    int rx_pin;                 /* GPIO for RX */
    uint32_t baudrate;          /* 0 = CRSF_BAUDRATE (420000) */
    size_t uart_rx_buf_size;    /* 0 = default (1024) */
    int uart_rx_full_thresh;    /* 0 = leave default; else uart_set_rx_full_threshold() */
    int uart_rx_timeout_thresh; /* 0 = leave default; else uart_set_rx_timeout() (UART driver units) */
    int uart_evt_queue_size;    /* 0 = default (16) UART event queue depth */
    int task_priority;          /* 0 = default (10) */
    int task_core;              /* -1 = no affinity */
    size_t task_stack_size;     /* 0 = default (4096) */
    crsf_rx_cb_t cb;            /* Frame callback */
    void *user_ctx;             /* Passed to callback */
} crsf_rx_config_t;

#define CRSF_RX_DEFAULT_CONFIG() { \
    .uart_port       = UART_NUM_1, \
    .rx_pin          = -1,         \
    .baudrate        = 0,          \
    .uart_rx_buf_size = 0,         \
    .uart_rx_full_thresh = 0,      \
    .uart_rx_timeout_thresh = 0,   \
    .uart_evt_queue_size = 0,      \
    .task_priority   = 0,          \
    .task_core       = -1,         \
    .task_stack_size = 0,          \
    .cb              = NULL,       \
    .user_ctx        = NULL,       \
}

/**
 * Initialize the CRSF receiver.
 * Configures UART and spawns a high-priority task that parses incoming frames.
 */
esp_err_t crsf_rx_init(const crsf_rx_config_t *config, crsf_rx_handle_t *out_handle);

/**
 * Stop the receiver task, release UART, and free resources.
 */
esp_err_t crsf_rx_deinit(crsf_rx_handle_t *handle);

#ifdef __cplusplus
}
#endif
