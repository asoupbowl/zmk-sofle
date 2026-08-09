/* SPDX-License-Identifier: MIT */

#pragma once

#include <stdint.h>

#define ZMK_CODEX_USAGE_UNKNOWN_PERCENT UINT8_MAX

struct zmk_codex_usage_state {
    uint8_t remaining_percent;
    uint8_t duration_hours;
    uint8_t reset_month;
    uint8_t reset_day;
    uint8_t reset_hour;
    uint8_t reset_minute;
};

struct zmk_codex_usage_state zmk_codex_usage_get_state(void);
void zmk_codex_usage_set_packed(uint32_t param1, uint32_t param2);
