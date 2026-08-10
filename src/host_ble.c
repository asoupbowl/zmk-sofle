/* SPDX-License-Identifier: MIT */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zmk/codex_usage/state.h>

LOG_MODULE_REGISTER(zmk_codex_usage_ble, CONFIG_ZMK_CODEX_USAGE_LOG_LEVEL);

#define CODEX_USAGE_SERVICE_UUID                                                                  \
    BT_UUID_128_ENCODE(0x9c3dfa10, 0x6a8b, 0x4c72, 0xbd2f, 0x19e56f2a7001)
#define CODEX_USAGE_SNAPSHOT_UUID                                                                 \
    BT_UUID_128_ENCODE(0x9c3dfa11, 0x6a8b, 0x4c72, 0xbd2f, 0x19e56f2a7001)

#define SNAPSHOT_SIZE 8

static void pack_snapshot(uint8_t payload[SNAPSHOT_SIZE]) {
    struct zmk_codex_usage_state state = zmk_codex_usage_get_state();
    uint32_t param1 = (uint32_t)state.remaining_percent | ((uint32_t)state.duration_hours << 8);
    uint32_t param2 = (uint32_t)state.reset_month | ((uint32_t)state.reset_day << 8) |
                      ((uint32_t)state.reset_hour << 16) |
                      ((uint32_t)state.reset_minute << 24);
    sys_put_le32(param1, payload);
    sys_put_le32(param2, payload + sizeof(param1));
}

static ssize_t read_snapshot(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                             uint16_t len, uint16_t offset) {
    uint8_t payload[SNAPSHOT_SIZE];
    pack_snapshot(payload);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, payload, sizeof(payload));
}

static ssize_t write_snapshot(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                              const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    if (len != SNAPSHOT_SIZE) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    const uint8_t *payload = buf;
    uint32_t param1 = sys_get_le32(payload);
    uint32_t param2 = sys_get_le32(payload + sizeof(param1));
    zmk_codex_usage_set_packed(param1, param2);
    LOG_DBG("Received Codex usage snapshot over BLE");
    return len;
}

BT_GATT_SERVICE_DEFINE(
    codex_usage_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(CODEX_USAGE_SERVICE_UUID)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(CODEX_USAGE_SNAPSHOT_UUID),
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
                               BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, read_snapshot,
                           write_snapshot, NULL));
