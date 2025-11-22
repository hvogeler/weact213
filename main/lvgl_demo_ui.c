/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/widgets/extra/meter.html#simple-meter

#include "lvgl.h"

static lv_obj_t *meter;

static void set_value(void *indic, int32_t v)
{
    lv_meter_set_indicator_end_value(meter, indic, v);
}

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    // Clear screen to white
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);

    // Create test pattern - rectangles in corners to verify orientation
    lv_obj_t *rect_tl = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect_tl, 30, 20);
    lv_obj_set_pos(rect_tl, 0, 0);
    lv_obj_set_style_bg_color(rect_tl, lv_color_black(), 0);
    lv_obj_clear_flag(rect_tl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *rect_tr = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect_tr, 30, 20);
    lv_obj_set_pos(rect_tr, 220, 0);  // 250-30 = 220
    lv_obj_set_style_bg_color(rect_tr, lv_color_black(), 0);
    lv_obj_clear_flag(rect_tr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *rect_bl = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect_bl, 30, 20);
    lv_obj_set_pos(rect_bl, 0, 102);  // 122-20 = 102
    lv_obj_set_style_bg_color(rect_bl, lv_color_black(), 0);
    lv_obj_clear_flag(rect_bl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *rect_br = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect_br, 30, 20);
    lv_obj_set_pos(rect_br, 220, 102);
    lv_obj_set_style_bg_color(rect_br, lv_color_black(), 0);
    lv_obj_clear_flag(rect_br, LV_OBJ_FLAG_SCROLLABLE);

    // Add text in center
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "WeAct 2.13\"");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // lv_obj_set_size(meter, 122, 250);
    // lv_obj_center(meter);

    // /*Create a scale for the minutes*/
    // /*61 ticks in a 360 degrees range (the last and the first line overlaps)*/
    // lv_meter_scale_t *scale_min = lv_meter_add_scale(meter);
    // lv_meter_set_scale_ticks(meter, scale_min, 61, 1, 10, lv_palette_main(LV_PALETTE_BLUE));
    // lv_meter_set_scale_range(meter, scale_min, 0, 60, 360, 270);

    // /*Create another scale for the hours. It's only visual and contains only major ticks*/
    // lv_meter_scale_t *scale_hour = lv_meter_add_scale(meter);
    // lv_meter_set_scale_ticks(meter, scale_hour, 12, 0, 0, lv_palette_main(LV_PALETTE_BLUE));               /*12 ticks*/
    // lv_meter_set_scale_major_ticks(meter, scale_hour, 1, 2, 20, lv_palette_main(LV_PALETTE_BLUE), 10);    /*Every tick is major*/
    // lv_meter_set_scale_range(meter, scale_hour, 1, 12, 330, 300);       /*[1..12] values in an almost full circle*/

    // LV_IMG_DECLARE(img_hand)

    // /*Add the hands from images*/
    // lv_meter_indicator_t *indic_min = lv_meter_add_needle_line(meter, scale_min, 4, lv_palette_main(LV_PALETTE_RED), -10);
    // lv_meter_indicator_t *indic_hour = lv_meter_add_needle_img(meter, scale_min, &img_hand, 5, 5);

    // /*Create an animation to set the value*/
    // lv_anim_t a;
    // lv_anim_init(&a);
    // lv_anim_set_exec_cb(&a, set_value);
    // lv_anim_set_values(&a, 0, 60);
    // lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    // lv_anim_set_time(&a, 60000 * 30);     /* 60 * 30 sec for 1 turn, 30 sec for 1 move of the minute hand*/
    // lv_anim_set_var(&a, indic_min);
    // lv_anim_start(&a);

    // lv_anim_set_var(&a, indic_hour);
    // lv_anim_set_time(&a, 60000 * 60 * 10);    /*36000 sec for 1 turn, 600 sec for 1 move of the hour hand*/
    // lv_anim_set_values(&a, 0, 60);
    // lv_anim_start(&a);
}
