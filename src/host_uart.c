/* SPDX-License-Identifier: MIT */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/codex_usage/state.h>

LOG_MODULE_REGISTER(zmk_codex_usage_uart, CONFIG_ZMK_CODEX_USAGE_LOG_LEVEL);

#define UART_NODE DT_CHOSEN(zmk_codex_usage_uart)
#define FRAME_MAX 40

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
static char rx_line[FRAME_MAX];
static size_t rx_len;
static char pending_line[FRAME_MAX];
static struct k_work sync_work;

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t value = 0;
    for (size_t i = 0; i < len; i++) {
        value ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            value = (value & 0x80) ? (value << 1) ^ 0x07 : value << 1;
        }
    }
    return value;
}

static void write_reply(const char *reply) {
    for (const char *p = reply; *p; p++) {
        uart_poll_out(uart_dev, *p);
    }
}

static int parse_frame(char *line, uint32_t *param1, uint32_t *param2) {
    if (strncmp(line, "CX1,", 4) != 0) {
        return -EINVAL;
    }
    char *star = strchr(line, '*');
    if (!star || strlen(star + 1) != 2) {
        return -EINVAL;
    }
    char *end;
    unsigned long received_crc = strtoul(star + 1, &end, 16);
    if (*end != '\0' || received_crc > UINT8_MAX ||
        crc8((const uint8_t *)line, star - line) != received_crc) {
        return -EBADMSG;
    }
    *star = '\0';
    char *separator = strchr(line + 4, ',');
    if (!separator) {
        return -EINVAL;
    }
    *separator = '\0';
    unsigned long first = strtoul(line + 4, &end, 16);
    if (*end != '\0' || first > UINT32_MAX) {
        return -EINVAL;
    }
    unsigned long second = strtoul(separator + 1, &end, 16);
    if (*end != '\0' || second > UINT32_MAX) {
        return -EINVAL;
    }
    *param1 = first;
    *param2 = second;
    return 0;
}

static void sync_work_handler(struct k_work *work) {
    char line[FRAME_MAX];
    unsigned int key = irq_lock();
    memcpy(line, pending_line, sizeof(line));
    irq_unlock(key);

    if (strcmp(line, "CX1?") == 0) {
        write_reply("CX1!\n");
        return;
    }

    uint32_t param1;
    uint32_t param2;
    int err = parse_frame(line, &param1, &param2);
    if (err) {
        LOG_WRN("Invalid Codex usage frame: %d", err);
        write_reply("CX1,ERR\n");
        return;
    }

    zmk_codex_usage_set_packed(param1, param2);
    write_reply("CX1,OK\n");
}

static void uart_callback(const struct device *dev, void *user_data) {
    if (!uart_irq_update(dev) || !uart_irq_rx_ready(dev)) {
        return;
    }
    uint8_t byte;
    while (uart_fifo_read(dev, &byte, 1) == 1) {
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            if (rx_len > 0) {
                rx_line[rx_len] = '\0';
                memcpy(pending_line, rx_line, rx_len + 1);
                k_work_submit(&sync_work);
            }
            rx_len = 0;
        } else if (rx_len < sizeof(rx_line) - 1) {
            rx_line[rx_len++] = byte;
        } else {
            rx_len = 0;
        }
    }
}

static int codex_usage_uart_init(void) {
    if (!device_is_ready(uart_dev)) {
        return -ENODEV;
    }
    k_work_init(&sync_work, sync_work_handler);
    int err = uart_irq_callback_user_data_set(uart_dev, uart_callback, NULL);
    if (err) {
        return err;
    }
    uart_irq_rx_enable(uart_dev);
    return 0;
}

SYS_INIT(codex_usage_uart_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
