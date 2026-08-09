/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <string.h>
#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/codex_usage/state.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/codex_usage_changed.h>

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

static void format_reset(char *out, size_t size, uint16_t minutes) {
    if (minutes == ZMK_CODEX_USAGE_UNKNOWN_MINUTES) {
        snprintf(out, size, "R --");
    } else if (minutes < 60) {
        snprintf(out, size, "R %uM", minutes);
    } else if (minutes < 1440) {
        snprintf(out, size, "R %uH", (minutes + 30) / 60);
    } else {
        snprintf(out, size, "R %uD", (minutes + 720) / 1440);
    }
}

static void draw_quota(lv_obj_t *canvas, lv_color_t buffer[], uint8_t hours, uint8_t used,
                       uint16_t reset_minutes) {
    lv_draw_rect_dsc_t bg;
    lv_draw_rect_dsc_t fg;
    lv_draw_label_dsc_t small;
    lv_draw_label_dsc_t large;
    init_rect(&bg, UI_BG);
    init_rect(&fg, UI_FG);
    init_label(&small, &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER);
    init_label(&large, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);

    lv_canvas_draw_rect(canvas, 0, 0, TILE_SIZE, TILE_SIZE, &bg);

    char window[5];
    char percent[8];
    char reset[8];
    format_window(window, sizeof(window), hours);
    format_reset(reset, sizeof(reset), reset_minutes);
    if (used == ZMK_CODEX_USAGE_UNKNOWN_PERCENT) {
        snprintf(percent, sizeof(percent), "--%%");
        used = 0;
    } else {
        snprintf(percent, sizeof(percent), "%u%%", used);
    }

    lv_canvas_draw_text(canvas, 0, 2, TILE_SIZE, &small, window);
    lv_canvas_draw_text(canvas, 0, 15, TILE_SIZE, &large, percent);
    lv_canvas_draw_rect(canvas, 4, 42, 60, 8, &fg);
    lv_canvas_draw_rect(canvas, 5, 43, 58, 6, &bg);
    if (used > 0) {
        uint8_t width = (58 * MIN(used, 100)) / 100;
        lv_canvas_draw_rect(canvas, 5, 43, width, 6, &fg);
    }
    lv_canvas_draw_text(canvas, 0, 55, TILE_SIZE, &small, reset);
    rotate_canvas(canvas, buffer);
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

static void update_widget(struct zmk_codex_usage_state state) {
    draw_quota(widget.primary, widget.primary_buf, state.primary_duration_hours,
               state.primary_used, state.primary_reset_minutes);
    draw_quota(widget.secondary, widget.secondary_buf, state.secondary_duration_hours,
               state.secondary_used, state.secondary_reset_minutes);
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
    widget_codex_usage_init();
    return 0;
}
