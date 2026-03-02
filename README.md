# esp-crsf-rx

[![Component Registry](https://components.espressif.com/components/manuel-alvarez-alvarez/esp-crsf-rx/badge.svg)](https://components.espressif.com/components/manuel-alvarez-alvarez/esp-crsf-rx)

High-performance [CRSF](https://github.com/crsf-wg/crsf/wiki) / [ELRS](https://www.expresslrs.org/) receiver decoder component for [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/).

## Features

- Decodes all 16 RC channels (11-bit, 0-2047) at up to 1000 Hz
- Parses 10 telemetry frame types: GPS, battery, attitude, link statistics, barometric altitude, variometer, heartbeat, flight mode, device ping, and device info
- CRC8 (DVB-S2) validation with automatic resynchronization on errors
- Callback-driven API
- Supports ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, and ESP32-H2

## Requirements

- ESP-IDF >= 5.0
- `CONFIG_FREERTOS_HZ=1000` (required for 1 ms UART read timeout)

## Installation

### ESP-IDF Component Registry

```bash
idf.py add-dependency "manuel-alvarez-alvarez/esp-crsf-rx"
```

### Manual

Clone or add this repository as a git submodule under your project's `components/` directory.

## Quick Start

```c
#include "crsf_rx.h"

static void on_crsf_message(const crsf_message_t *msg, void *ctx) {
    if (msg->type == CRSF_FRAMETYPE_RC_CHANNELS_PACKED) {
        printf("CH1=%u CH2=%u\n", msg->channels.ch[0], msg->channels.ch[1]);
    }
}

void app_main(void) {
    crsf_rx_config_t cfg = CRSF_RX_DEFAULT_CONFIG();
    cfg.rx_pin    = GPIO_NUM_13;
    cfg.cb        = on_crsf_message;

    crsf_rx_handle_t rx;
    ESP_ERROR_CHECK(crsf_rx_init(&cfg, &rx));
}
```

A full example is available in [`examples/basic`](examples/basic).

## API

### Configuration

Use `CRSF_RX_DEFAULT_CONFIG()` and override only the fields you need.

| Field | Type | Default | Description |
|---|---|---|---|
| `uart_port` | `uart_port_t` | `UART_NUM_1` | UART peripheral to use |
| `rx_pin` | `int` | `-1` | GPIO pin for CRSF input (**required**) |
| `baudrate` | `uint32_t` | `420000` | CRSF baud rate |
| `uart_rx_buf_size` | `size_t` | `1024` | UART RX ring buffer size in bytes |
| `uart_rx_full_thresh` | `int` | driver default | UART RX FIFO full threshold |
| `uart_rx_timeout_thresh` | `int` | driver default | UART RX timeout threshold (UART symbol units) |
| `read_timeout_ms` | `uint32_t` | `1` | `uart_read_bytes()` timeout in ms |
| `task_priority` | `int` | `10` | FreeRTOS task priority |
| `task_core` | `int` | no affinity | Pin task to core (`-1` = any) |
| `task_stack_size` | `size_t` | `4096` | Task stack size in bytes |
| `cb` | `crsf_rx_cb_t` | `NULL` | Frame callback (optional) |
| `user_ctx` | `void *` | `NULL` | User context passed to callback |

Fields set to `0` (or `-1` for `task_core`) use their defaults.

### Functions

| Function | Description |
|---|---|
| `crsf_rx_init(config, &handle)` | Initialize the receiver; starts the UART and background task |
| `crsf_rx_deinit(&handle)` | Stop the task, release UART and memory |

### Message Types

The callback receives a `crsf_message_t` tagged union. Switch on `msg->type`:

| Type | Union field | Description |
|---|---|---|
| `CRSF_FRAMETYPE_RC_CHANNELS_PACKED` | `channels` | 16 RC channels (0-2047) |
| `CRSF_FRAMETYPE_LINK_STATISTICS` | `link_stats` | RSSI, LQ, SNR, RF mode, TX power |
| `CRSF_FRAMETYPE_GPS` | `gps` | Lat, lon, groundspeed, heading, altitude, satellites |
| `CRSF_FRAMETYPE_BATTERY_SENSOR` | `battery` | Voltage, current, capacity, remaining % |
| `CRSF_FRAMETYPE_ATTITUDE` | `attitude` | Pitch, roll, yaw |
| `CRSF_FRAMETYPE_BARO_ALTITUDE` | `baro_altitude` | Barometric altitude and optional vario |
| `CRSF_FRAMETYPE_VARIO` | `vario` | Vertical speed |
| `CRSF_FRAMETYPE_HEARTBEAT` | `heartbeat` | Origin address |
| `CRSF_FRAMETYPE_FLIGHT_MODE` | `flight_mode` | Mode string |
| `CRSF_FRAMETYPE_DEVICE_PING` | `device_ping` | Destination and origin |
| `CRSF_FRAMETYPE_DEVICE_INFO` | `device_info` | Device name, serial, HW/FW IDs |

See [`include/crsf_protocol.h`](include/crsf_protocol.h) for full struct definitions.

## Testing

Unit tests run on the host (no hardware required):

```bash
cmake -B test/build test
cmake --build test/build
ctest --test-dir test/build --output-on-failure
```

## License

[MIT](LICENSE)