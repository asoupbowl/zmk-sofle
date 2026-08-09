/* SPDX-License-Identifier: MIT */

#pragma once

#include <zmk/codex_usage/state.h>
#include <zmk/event_manager.h>

struct zmk_codex_usage_changed {
    struct zmk_codex_usage_state state;
};

ZMK_EVENT_DECLARE(zmk_codex_usage_changed);
