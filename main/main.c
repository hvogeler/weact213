/**
 * @file main.c
 * @brief WeAct E-Paper Display Example with LVGL 9
 *
 * This example demonstrates how to use the WeAct 2.13" e-paper display
 * with LVGL 9 graphics library.
 *
 * Hardware:
 * - WeAct Studio 2.13" E-Paper Display (122x250 pixels)
 * - ESP32-S3 DevKit
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_weact_epaper.h"

static const char *TAG = "epaper_main";

LV_FONT_DECLARE(Rubik_Medium_48)
LV_FONT_DECLARE(Rubik_Regular_36)

/**
 * @brief Create simple UI demonstration
 *
 * Creates a basic UI with:
 * - Title label
 * - Status text
 * - Simple shapes
 */
static void create_demo_ui(void)
{
    // Get the active screen
    lv_obj_t *scr = lv_screen_active();

    // Create title label
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "21.5");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 10);
    lv_obj_set_style_text_font(title, &Rubik_Medium_48, 0);

    // Create status label (landscape: 250x122)
    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "LVGL 9.4.0\n250x122 pixels\nLandscape Mode");
    lv_obj_align_to(status, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);
    lv_obj_set_style_text_font(status, &Rubik_Regular_36, 0);

    // Create a simple rectangle (wider for landscape)
    // lv_obj_t *rect = lv_obj_create(scr);
    // lv_obj_set_size(rect, 100, 40);
    // lv_obj_align(rect, LV_ALIGN_BOTTOM_MID, 0, -10);
    // lv_obj_set_style_bg_color(rect, lv_color_white(), 0);

    ESP_LOGI(TAG, "Demo UI created");
}

static void ui_init()
{
    lv_obj_t *main_view_ = lv_screen_active();
    lv_obj_clean(main_view_);
    lv_obj_set_style_bg_color(main_view_, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_pad_all(main_view_, 0, LV_PART_MAIN);

    // Create current temp
    lv_obj_t *lbl_cur_temp = lv_label_create(main_view_);
    lv_label_set_text(lbl_cur_temp, "Current Temp °C");

    lv_obj_set_width(lbl_cur_temp, lv_pct(100));
    lv_obj_set_style_text_font(lbl_cur_temp, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_cur_temp, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_pos(lbl_cur_temp, 0, 0);

    lv_obj_t *cur_temp_ = lv_label_create(main_view_);
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.1f", 24.5);
    lv_label_set_text_fmt(cur_temp_, "%s", temp_str);
    lv_obj_set_width(cur_temp_, lv_pct(100));
    lv_obj_set_style_text_font(cur_temp_, &Rubik_Medium_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(cur_temp_, lv_color_black(), LV_PART_MAIN);

    lv_obj_align_to(cur_temp_, lbl_cur_temp, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -4);

    // Create target temp
    lv_obj_t *lbl_tgt_temp = lv_label_create(main_view_);
    lv_label_set_text(lbl_tgt_temp, "Target Temp °C");
    lv_obj_set_width(lbl_tgt_temp, lv_pct(100));
    lv_obj_set_style_text_font(lbl_tgt_temp, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_tgt_temp, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_tgt_temp, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_pos(lbl_tgt_temp, 0, 0);

    lv_obj_align_to(lbl_tgt_temp, cur_temp_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -6);

    lv_obj_t *tgt_temp_ = lv_label_create(main_view_);
    snprintf(temp_str, sizeof(temp_str), "%.1f", 21.0);
    lv_label_set_text_fmt(tgt_temp_, "%s", temp_str);
    lv_obj_set_width(tgt_temp_, lv_pct(100));
    lv_obj_set_style_text_font(tgt_temp_, &Rubik_Medium_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(tgt_temp_, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_align(tgt_temp_, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    lv_obj_align_to(tgt_temp_, lbl_tgt_temp, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -4);

    // Version label
    lv_obj_t *label_version = lv_label_create(main_view_);
    lv_label_set_text_fmt(label_version, "v%s  %s",
                          CONFIG_APP_PROJECT_VER,
                          "vogeler2129");
    lv_obj_set_style_text_font(label_version, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_version, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(label_version, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    // xTaskCreatePinnedToCore(update_task, "update_task", 4096 * 2, NULL, 0, NULL, 1);
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting WeAct E-Paper LVGL 9 Example");

    // ===============================================
    // Step 1: Initialize LVGL
    // ===============================================
    ESP_LOGI(TAG, "Initializing LVGL 9...");
    lv_init();

    // ===============================================
    // Step 2: Create display with landscape config
    // ===============================================
    ESP_LOGI(TAG, "Creating WeAct E-Paper display...");

    lvgl_weact_epaper_config_t config = lvgl_weact_epaper_get_default_config();
    config.landscape = true; // Enable landscape mode (250x122)

    lv_display_t *disp = lvgl_weact_epaper_create(&config);
    if (disp == NULL)
    {
        ESP_LOGE(TAG, "Failed to create display!");
        return;
    }

    ESP_LOGI(TAG, "Display created successfully");

    // ===============================================
    // Step 3: Set up LVGL tick timer
    // ===============================================
    // ESP_LOGI(TAG, "Setting up LVGL tick timer...");

    // const esp_timer_create_args_t lvgl_tick_timer_args = {
    //     .callback = lvgl_tick_timer_cb,
    //     .name = "lvgl_tick"
    // };

    // esp_timer_handle_t lvgl_tick_timer = NULL;
    // ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    // ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 10 * 1000));  // 10ms

    // ===============================================
    // Step 4: Create UI
    // ===============================================
    ESP_LOGI(TAG, "Creating demo UI...");
    // create_demo_ui();
    ui_init();

    // ===============================================
    // Step 5: Main LVGL task loop
    // ===============================================
    ESP_LOGI(TAG, "Entering main loop...");

    while (1)
    {
        // Handle LVGL tasks (rendering, input, etc.)
        uint32_t time_till_next = lv_timer_handler();

        // Sleep for the recommended time
        vTaskDelay(pdMS_TO_TICKS(time_till_next > 100 ? 100 : time_till_next));
    }
}
