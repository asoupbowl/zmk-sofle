/* SPDX-License-Identifier: MIT */

#include <zmk/events/codex_usage_changed.h>

ZMK_EVENT_IMPL(zmk_codex_usage_changed);

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
ZMK_RELAY_EVENT_HANDLE(zmk_codex_usage_changed, cx, );
ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(zmk_codex_usage_changed, cx, );
#endif
