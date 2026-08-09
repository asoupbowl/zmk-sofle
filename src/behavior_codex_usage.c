/* SPDX-License-Identifier: MIT */

#define DT_DRV_COMPAT zmk_behavior_codex_usage

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zmk/codex_usage/state.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int codex_usage_pressed(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    zmk_codex_usage_set_packed(binding->param1, binding->param2);
    return 0;
}

static int codex_usage_released(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    return 0;
}

static const struct behavior_driver_api codex_usage_driver_api = {
    .binding_pressed = codex_usage_pressed,
    .binding_released = codex_usage_released,
};

#define CODEX_USAGE_INST(n)                                                                       \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                               \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &codex_usage_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CODEX_USAGE_INST)

#endif
