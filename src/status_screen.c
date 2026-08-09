/* SPDX-License-Identifier: MIT */

#include <lvgl.h>
#include <zmk/display/status_screen.h>

int zmk_codex_usage_widget_init(lv_obj_t *parent);

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    zmk_codex_usage_widget_init(screen);
    return screen;
}
