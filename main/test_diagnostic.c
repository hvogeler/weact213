/**
 * @file test_diagnostic.c
 * @brief Diagnostic test to identify display orientation issues
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680_lowlevel.h"
#include <string.h>

static const char *TAG = "DIAG";

#define PIN_SPI_SCK     6
#define PIN_SPI_MOSI    7
#define PIN_EPD_CS      10
#define PIN_EPD_DC      9
#define PIN_EPD_RST     4
#define PIN_EPD_BUSY    18
#define SPI_CLOCK_HZ    (4 * 1000 * 1000)

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "SSD1680 Diagnostic Test");
    ESP_LOGI(TAG, "=================================================");

    ssd1680_config_t config = {
        .pin_sck = PIN_SPI_SCK,
        .pin_mosi = PIN_SPI_MOSI,
        .pin_cs = PIN_EPD_CS,
        .pin_dc = PIN_EPD_DC,
        .pin_rst = PIN_EPD_RST,
        .pin_busy = PIN_EPD_BUSY,
        .spi_clock_speed_hz = SPI_CLOCK_HZ,
    };

    ssd1680_t display;
    if (!ssd1680_init(&display, &config)) {
        ESP_LOGE(TAG, "Init failed!");
        return;
    }

    // =========================================================================
    // TEST 1: Four corners - identify which corner is which
    // =========================================================================
    ESP_LOGI(TAG, "TEST 1: Drawing pixels in four corners");
    ESP_LOGI(TAG, "This will help identify the display orientation");

    ssd1680_clear_screen(&display);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Draw 10x10 squares in each corner
    // Top-left corner
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    // Top-right corner
    for (int y = 0; y < 10; y++) {
        for (int x = SSD1680_WIDTH - 10; x < SSD1680_WIDTH; x++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    // Bottom-left corner
    for (int y = SSD1680_HEIGHT - 10; y < SSD1680_HEIGHT; y++) {
        for (int x = 0; x < 10; x++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    // Bottom-right corner
    for (int y = SSD1680_HEIGHT - 10; y < SSD1680_HEIGHT; y++) {
        for (int x = SSD1680_WIDTH - 10; x < SSD1680_WIDTH; x++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "You should see 4 black squares in the corners");
    ESP_LOGI(TAG, "Note which physical corners they appear in!");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 2: Horizontal lines - test X addressing
    // =========================================================================
    ESP_LOGI(TAG, "TEST 2: Drawing horizontal lines");

    memset(display.framebuffer, 0xFF, SSD1680_BUFFER_SIZE);

    // Draw horizontal lines every 20 pixels
    for (int y = 0; y < SSD1680_HEIGHT; y += 20) {
        for (int x = 0; x < SSD1680_WIDTH; x++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "You should see horizontal lines across the display");
    ESP_LOGI(TAG, "Are they truly horizontal on your physical display?");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 3: Vertical lines - test Y addressing
    // =========================================================================
    ESP_LOGI(TAG, "TEST 3: Drawing vertical lines");

    memset(display.framebuffer, 0xFF, SSD1680_BUFFER_SIZE);

    // Draw vertical lines every 20 pixels
    for (int x = 0; x < SSD1680_WIDTH; x += 20) {
        for (int y = 0; y < SSD1680_HEIGHT; y++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "You should see vertical lines down the display");
    ESP_LOGI(TAG, "Are they truly vertical on your physical display?");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 4: Half screen test - LEFT half black
    // =========================================================================
    ESP_LOGI(TAG, "TEST 4: Left half black");

    memset(display.framebuffer, 0xFF, SSD1680_BUFFER_SIZE);

    // Fill left half (X: 0 to WIDTH/2)
    for (int y = 0; y < SSD1680_HEIGHT; y++) {
        for (int x = 0; x < SSD1680_WIDTH / 2; x++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "LEFT half should be black, RIGHT half white");
    ESP_LOGI(TAG, "What do you actually see?");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 5: Half screen test - TOP half black
    // =========================================================================
    ESP_LOGI(TAG, "TEST 5: Top half black");

    memset(display.framebuffer, 0xFF, SSD1680_BUFFER_SIZE);

    // Fill top half (Y: 0 to HEIGHT/2)
    for (int y = 0; y < SSD1680_HEIGHT / 2; y++) {
        for (int x = 0; x < SSD1680_WIDTH; x++) {
            ssd1680_draw_pixel(&display, x, y, 1);
        }
    }

    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "TOP half should be black, BOTTOM half white");
    ESP_LOGI(TAG, "What do you actually see?");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 6: Diagonal line
    // =========================================================================
    ESP_LOGI(TAG, "TEST 6: Diagonal line");

    memset(display.framebuffer, 0xFF, SSD1680_BUFFER_SIZE);

    // Draw diagonal from (0,0) to (width, height)
    for (int i = 0; i < SSD1680_HEIGHT; i++) {
        int x = (i * SSD1680_WIDTH) / SSD1680_HEIGHT;
        ssd1680_draw_pixel(&display, x, i, 1);
    }

    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "You should see a diagonal line from top-left to bottom-right");
    ESP_LOGI(TAG, "Is it at the expected 45-degree angle?");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // Report Results
    // =========================================================================
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Diagnostic Complete!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Please report what you saw:");
    ESP_LOGI(TAG, "1. Where did the 4 corner squares appear?");
    ESP_LOGI(TAG, "2. Were the 'horizontal' lines actually horizontal?");
    ESP_LOGI(TAG, "3. Were the 'vertical' lines actually vertical?");
    ESP_LOGI(TAG, "4. Which half was black in test 4?");
    ESP_LOGI(TAG, "5. Which half was black in test 5?");
    ESP_LOGI(TAG, "6. Was the diagonal at 45 degrees?");
    ESP_LOGI(TAG, "=================================================");

    ssd1680_sleep(&display);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
