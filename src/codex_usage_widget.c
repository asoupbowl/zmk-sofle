/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <string.h>
#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/battery.h>
#include <zmk/codex_usage/state.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/codex_usage_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/bluetooth/peripheral.h>

#define TILE_SIZE 68

#define UI_BG                                                                                      \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white()
#define UI_FG                                                                                      \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black()

struct codex_usage_widget {
    lv_obj_t *root;
    lv_obj_t *primary;
    lv_obj_t *secondary;
    lv_obj_t *footer;
    lv_color_t primary_buf[TILE_SIZE * TILE_SIZE];
    lv_color_t secondary_buf[TILE_SIZE * TILE_SIZE];
    lv_color_t footer_buf[TILE_SIZE * TILE_SIZE];
};

static struct codex_usage_widget widget;
static lv_color_t rotate_buf[TILE_SIZE * TILE_SIZE];

struct codex_header_state {
    uint8_t battery;
    bool connected;
};

static struct codex_header_state header_state;

static void init_label(lv_draw_label_dsc_t *dsc, const lv_font_t *font,
                       lv_text_align_t align) {
    lv_draw_label_dsc_init(dsc);
    dsc->color = UI_FG;
    dsc->font = font;
    dsc->align = align;
}

static void init_rect(lv_draw_rect_dsc_t *dsc, lv_color_t color) {
    lv_draw_rect_dsc_init(dsc);
    dsc->bg_color = color;
    dsc->border_width = 0;
}

static void rotate_canvas(lv_obj_t *canvas, lv_color_t source[]) {
    memcpy(rotate_buf, source, sizeof(rotate_buf));
    lv_img_dsc_t image = {0};
    image.data = (void *)rotate_buf;
    image.header.cf = LV_IMG_CF_TRUE_COLOR;
    image.header.w = TILE_SIZE;
    image.header.h = TILE_SIZE;
    lv_canvas_fill_bg(canvas, UI_BG, LV_OPA_COVER);
    lv_canvas_transform(canvas, &image, 900, LV_IMG_ZOOM_NONE, -1, 0, TILE_SIZE / 2,
                        TILE_SIZE / 2, true);
}

static void format_window(char *out, size_t size, uint8_t hours) {
    if (hours == 0) {
        snprintf(out, size, "--");
    } else if (hours < 24) {
        snprintf(out, size, "%uH", hours);
    } else {
        snprintf(out, size, "%uD", (hours + 12) / 24);
    }
}

static void draw_usage(struct zmk_codex_usage_state state) {
    lv_draw_rect_dsc_t bg;
    lv_draw_rect_dsc_t fg;
    lv_draw_label_dsc_t small;
    lv_draw_label_dsc_t large;
    lv_draw_label_dsc_t icon;
    init_rect(&bg, UI_BG);
    init_rect(&fg, UI_FG);
    init_label(&small, &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER);
    init_label(&large, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    init_label(&icon, &lv_font_montserrat_16, LV_TEXT_ALIGN_LEFT);

    lv_canvas_draw_rect(widget.primary, 0, 0, TILE_SIZE, TILE_SIZE, &bg);

    char window[5];
    char percent[8];
    format_window(window, sizeof(window), state.duration_hours);
    uint8_t remaining = state.remaining_percent;
    if (remaining == ZMK_CODEX_USAGE_UNKNOWN_PERCENT) {
        snprintf(percent, sizeof(percent), "--%%");
        remaining = 0;
    } else {
        snprintf(percent, sizeof(percent), "%u%%", remaining);
    }

    lv_canvas_draw_text(widget.primary, 0, 0, 16, &icon,
                        header_state.connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
    lv_canvas_draw_text(widget.primary, 20, 3, 28, &small, window);

    lv_canvas_draw_rect(widget.primary, 51, 4, 13, 9, &fg);
    lv_canvas_draw_rect(widget.primary, 52, 5, 11, 7, &bg);
    lv_canvas_draw_rect(widget.primary, 49, 6, 2, 3, &fg);
    uint8_t battery_width = (11 * MIN(header_state.battery, 100)) / 100;
    if (battery_width > 0) {
        lv_canvas_draw_rect(widget.primary, 52, 5, battery_width, 7, &fg);
    }

    lv_canvas_draw_text(widget.primary, 0, 17, TILE_SIZE, &large, percent);
    lv_canvas_draw_rect(widget.primary, 4, 42, 60, 8, &fg);
    lv_canvas_draw_rect(widget.primary, 5, 43, 58, 6, &bg);
    if (remaining > 0) {
        uint8_t width = (58 * MIN(remaining, 100)) / 100;
        lv_canvas_draw_rect(widget.primary, 5, 43, width, 6, &fg);
    }
    lv_canvas_draw_text(widget.primary, 0, 55, TILE_SIZE, &small, "LEFT");
    rotate_canvas(widget.primary, widget.primary_buf);
}

static void draw_reset(struct zmk_codex_usage_state state) {
    lv_draw_rect_dsc_t bg;
    lv_draw_label_dsc_t small;
    lv_draw_label_dsc_t large;
    init_rect(&bg, UI_BG);
    init_label(&small, &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER);
    init_label(&large, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    lv_canvas_draw_rect(widget.secondary, 0, 0, TILE_SIZE, TILE_SIZE, &bg);

    char date[8];
    char time[8];
    if (state.reset_month == 0 || state.reset_day == 0) {
        snprintf(date, sizeof(date), "--/--");
        snprintf(time, sizeof(time), "--:--");
    } else {
        snprintf(date, sizeof(date), "%02u/%02u", state.reset_month, state.reset_day);
        snprintf(time, sizeof(time), "%02u:%02u", state.reset_hour, state.reset_minute);
    }

    lv_canvas_draw_text(widget.secondary, 0, 3, TILE_SIZE, &small, "RESET");
    lv_canvas_draw_text(widget.secondary, 0, 19, TILE_SIZE, &large, date);
    lv_canvas_draw_text(widget.secondary, 0, 51, TILE_SIZE, &small, time);
    rotate_canvas(widget.secondary, widget.secondary_buf);
}

static void draw_footer(void) {
    lv_draw_rect_dsc_t bg;
    lv_draw_label_dsc_t title;
    init_rect(&bg, UI_BG);
    init_label(&title, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    lv_canvas_draw_rect(widget.footer, 0, 0, TILE_SIZE, TILE_SIZE, &bg);
    lv_canvas_draw_text(widget.footer, 0, 1, TILE_SIZE, &title, "CODEX");
    rotate_canvas(widget.footer, widget.footer_buf);
}

static void header_update_cb(struct codex_header_state state) {
    header_state = state;
    draw_usage(zmk_codex_usage_get_state());
}

static struct codex_header_state header_get_state(const zmk_event_t *eh) {
    return (struct codex_header_state){
        .battery = zmk_battery_state_of_charge(),
        .connected = zmk_split_bt_peripheral_is_connected(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_codex_header, struct codex_header_state, header_update_cb,
                            header_get_state)
ZMK_SUBSCRIPTION(widget_codex_header, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(widget_codex_header, zmk_split_peripheral_status_changed);

static void update_widget(struct zmk_codex_usage_state state) {
    draw_usage(state);
    draw_reset(state);
}

static void widget_update_cb(struct zmk_codex_usage_state state) { update_widget(state); }

static struct zmk_codex_usage_state widget_get_state(const zmk_event_t *eh) {
    const struct zmk_codex_usage_changed *event = as_zmk_codex_usage_changed(eh);
    return event ? event->state : zmk_codex_usage_get_state();
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_codex_usage, struct zmk_codex_usage_state, widget_update_cb,
                            widget_get_state)
ZMK_SUBSCRIPTION(widget_codex_usage, zmk_codex_usage_changed);

int zmk_codex_usage_widget_init(lv_obj_t *parent) {
    widget.root = lv_obj_create(parent);
    lv_obj_remove_style_all(widget.root);
    lv_obj_set_size(widget.root, 160, 68);
    lv_obj_clear_flag(widget.root, LV_OBJ_FLAG_SCROLLABLE);

    widget.primary = lv_canvas_create(widget.root);
    lv_obj_align(widget.primary, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(widget.primary, widget.primary_buf, TILE_SIZE, TILE_SIZE,
                         LV_IMG_CF_TRUE_COLOR);

    widget.secondary = lv_canvas_create(widget.root);
    lv_obj_align(widget.secondary, LV_ALIGN_TOP_LEFT, 24, 0);
    lv_canvas_set_buffer(widget.secondary, widget.secondary_buf, TILE_SIZE, TILE_SIZE,
                         LV_IMG_CF_TRUE_COLOR);

    widget.footer = lv_canvas_create(widget.root);
    lv_obj_align(widget.footer, LV_ALIGN_TOP_LEFT, -44, 0);
    lv_canvas_set_buffer(widget.footer, widget.footer_buf, TILE_SIZE, TILE_SIZE,
                         LV_IMG_CF_TRUE_COLOR);

    draw_footer();
    widget_codex_header_init();
    widget_codex_usage_init();
    return 0;
}
