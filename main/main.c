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

/**
 * @brief LVGL tick timer callback
 *
 * LVGL requires a periodic tick to handle timing, animations, etc.
 */
static void lvgl_tick_timer_cb(void *arg)
{
    lv_tick_inc(10);  // 10ms tick
}

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
    lv_label_set_text(title, "WeAct E-Paper");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Create status label
    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "LVGL 9.4.0\n122x250 pixels");
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 0);

    // Create a simple rectangle
    lv_obj_t *rect = lv_obj_create(scr);
    lv_obj_set_size(rect, 80, 60);
    lv_obj_align(rect, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(rect, lv_color_black(), 0);

    ESP_LOGI(TAG, "Demo UI created");
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
    // Step 2: Create display with default config
    // ===============================================
    ESP_LOGI(TAG, "Creating WeAct E-Paper display...");

    lvgl_weact_epaper_config_t config = lvgl_weact_epaper_get_default_config();

    lv_display_t *disp = lvgl_weact_epaper_create(&config);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to create display!");
        return;
    }

    ESP_LOGI(TAG, "Display created successfully");

    // ===============================================
    // Step 3: Set up LVGL tick timer
    // ===============================================
    ESP_LOGI(TAG, "Setting up LVGL tick timer...");

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = lvgl_tick_timer_cb,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 10 * 1000));  // 10ms

    // ===============================================
    // Step 4: Create UI
    // ===============================================
    ESP_LOGI(TAG, "Creating demo UI...");
    create_demo_ui();

    // ===============================================
    // Step 5: Main LVGL task loop
    // ===============================================
    ESP_LOGI(TAG, "Entering main loop...");

    while (1) {
        // Handle LVGL tasks (rendering, input, etc.)
        uint32_t time_till_next = lv_timer_handler();

        // Sleep for the recommended time
        vTaskDelay(pdMS_TO_TICKS(time_till_next > 100 ? 100 : time_till_next));
    }
}
