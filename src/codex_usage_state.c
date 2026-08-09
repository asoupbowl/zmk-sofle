/* SPDX-License-Identifier: MIT */

#include <zephyr/kernel.h>
#include <zmk/codex_usage/state.h>
#include <zmk/events/codex_usage_changed.h>

K_MUTEX_DEFINE(codex_usage_mutex);

static struct zmk_codex_usage_state state = {
    .primary_used = ZMK_CODEX_USAGE_UNKNOWN_PERCENT,
    .secondary_used = ZMK_CODEX_USAGE_UNKNOWN_PERCENT,
    .primary_reset_minutes = ZMK_CODEX_USAGE_UNKNOWN_MINUTES,
    .secondary_reset_minutes = ZMK_CODEX_USAGE_UNKNOWN_MINUTES,
};

struct zmk_codex_usage_state zmk_codex_usage_get_state(void) {
    k_mutex_lock(&codex_usage_mutex, K_FOREVER);
    struct zmk_codex_usage_state copy = state;
    k_mutex_unlock(&codex_usage_mutex);
    return copy;
}

void zmk_codex_usage_set_packed(uint32_t param1, uint32_t param2) {
    struct zmk_codex_usage_state updated = {
        .primary_used = param1 & 0xFF,
        .secondary_used = (param1 >> 8) & 0xFF,
        .primary_duration_hours = (param1 >> 16) & 0xFF,
        .secondary_duration_hours = (param1 >> 24) & 0xFF,
        .primary_reset_minutes = param2 & 0xFFFF,
        .secondary_reset_minutes = (param2 >> 16) & 0xFFFF,
    };
    k_mutex_lock(&codex_usage_mutex, K_FOREVER);
    state = updated;
    k_mutex_unlock(&codex_usage_mutex);
    raise_zmk_codex_usage_changed((struct zmk_codex_usage_changed){.state = updated});
}
