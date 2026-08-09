/* SPDX-License-Identifier: MIT */

#include <zephyr/kernel.h>
#include <zmk/codex_usage/state.h>
#include <zmk/events/codex_usage_changed.h>

K_MUTEX_DEFINE(codex_usage_mutex);

static struct zmk_codex_usage_state state = {
    .remaining_percent = ZMK_CODEX_USAGE_UNKNOWN_PERCENT,
};

struct zmk_codex_usage_state zmk_codex_usage_get_state(void) {
    k_mutex_lock(&codex_usage_mutex, K_FOREVER);
    struct zmk_codex_usage_state copy = state;
    k_mutex_unlock(&codex_usage_mutex);
    return copy;
}

void zmk_codex_usage_set_packed(uint32_t param1, uint32_t param2) {
    struct zmk_codex_usage_state updated = {
        .remaining_percent = param1 & 0xFF,
        .duration_hours = (param1 >> 8) & 0xFF,
        .reset_month = param2 & 0xFF,
        .reset_day = (param2 >> 8) & 0xFF,
        .reset_hour = (param2 >> 16) & 0xFF,
        .reset_minute = (param2 >> 24) & 0xFF,
    };
    k_mutex_lock(&codex_usage_mutex, K_FOREVER);
    state = updated;
    k_mutex_unlock(&codex_usage_mutex);
    raise_zmk_codex_usage_changed((struct zmk_codex_usage_changed){.state = updated});
}
