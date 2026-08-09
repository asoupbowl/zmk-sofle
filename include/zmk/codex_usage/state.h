/* SPDX-License-Identifier: MIT */

#pragma once

#include <stdint.h>

#define ZMK_CODEX_USAGE_UNKNOWN_PERCENT UINT8_MAX
#define ZMK_CODEX_USAGE_UNKNOWN_MINUTES UINT16_MAX

struct zmk_codex_usage_state {
    uint8_t primary_used;
    uint8_t secondary_used;
    uint8_t primary_duration_hours;
    uint8_t secondary_duration_hours;
    uint16_t primary_reset_minutes;
    uint16_t secondary_reset_minutes;
};

struct zmk_codex_usage_state zmk_codex_usage_get_state(void);
void zmk_codex_usage_set_packed(uint32_t param1, uint32_t param2);
