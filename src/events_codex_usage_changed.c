/* SPDX-License-Identifier: MIT */

#include <zephyr/logging/log.h>
#include <zmk/events/codex_usage_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

ZMK_EVENT_IMPL(zmk_codex_usage_changed);

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
ZMK_RELAY_EVENT_HANDLE(zmk_codex_usage_changed, cx, );
ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(zmk_codex_usage_changed, cx, );
#endif
