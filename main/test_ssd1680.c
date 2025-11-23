/**
 * @file test_ssd1680.c
 * @brief Test program for SSD1680 e-paper display driver
 *
 * This program tests the SSD1680 controller (common in WeAct 2.13" displays)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1680_lowlevel.h"

static const char *TAG = "TEST";

// =============================================================================
// PIN CONFIGURATION - Update these to match your wiring!
// =============================================================================

#define PIN_SPI_SCK     6
#define PIN_SPI_MOSI    7
#define PIN_EPD_CS      10
#define PIN_EPD_DC      9
#define PIN_EPD_RST     4
#define PIN_EPD_BUSY    18

#define SPI_CLOCK_HZ    (4 * 1000 * 1000)  // 4 MHz

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

void draw_checkerboard(ssd1680_t *dev, int square_size)
{
    ESP_LOGI(TAG, "Drawing checkerboard pattern (square_size=%d)", square_size);

    for (int y = 0; y < SSD1680_HEIGHT; y++) {
        for (int x = 0; x < SSD1680_WIDTH; x++) {
            int grid_x = x / square_size;
            int grid_y = y / square_size;
            bool is_black = ((grid_x + grid_y) % 2) == 0;

            ssd1680_draw_pixel(dev, x, y, is_black ? 1 : 0);
        }
    }
}

void draw_large_digit(ssd1680_t *dev, int start_x, int start_y, int digit, int scale)
{
    const uint8_t font_5x7[][7] = {
        {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110},  // 0
        {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},  // 1
        {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111},  // 2
        {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110},  // 3
    };

    if (digit < 0 || digit > 3) return;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (font_5x7[digit][row] & (1 << (4 - col))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        ssd1680_draw_pixel(dev,
                                         start_x + col * scale + sx,
                                         start_y + row * scale + sy,
                                         1);
                    }
                }
            }
        }
    }
}

void draw_test_pattern(ssd1680_t *dev)
{
    ESP_LOGI(TAG, "Drawing comprehensive test pattern");

    // Border around entire screen
    ssd1680_draw_rectangle(dev, 0, 0, SSD1680_WIDTH - 1, SSD1680_HEIGHT - 1, false);

    // Filled square in center
    int square_size = 60;
    int center_x = SSD1680_WIDTH / 2;
    int center_y = SSD1680_HEIGHT / 2;
    ssd1680_draw_rectangle(dev,
                          center_x - square_size/2,
                          center_y - square_size/2,
                          center_x + square_size/2,
                          center_y + square_size/2,
                          true);

    // Outline square around center
    ssd1680_draw_rectangle(dev,
                          center_x - square_size/2 - 10,
                          center_y - square_size/2 - 10,
                          center_x + square_size/2 + 10,
                          center_y + square_size/2 + 10,
                          false);

    // Corner labels
    draw_large_digit(dev, 10, 10, 1, 3);                                    // Top-left
    draw_large_digit(dev, SSD1680_WIDTH - 30, 10, 2, 3);                   // Top-right
    draw_large_digit(dev, 10, SSD1680_HEIGHT - 30, 3, 3);                  // Bottom-left
    draw_large_digit(dev, SSD1680_WIDTH - 30, SSD1680_HEIGHT - 30, 0, 3);  // Bottom-right

    // Horizontal line across middle
    for (int x = 40; x < SSD1680_WIDTH - 40; x++) {
        ssd1680_draw_pixel(dev, x, center_y, 1);
    }

    // Vertical line down middle
    for (int y = 40; y < SSD1680_HEIGHT - 40; y++) {
        ssd1680_draw_pixel(dev, center_x, y, 1);
    }
}

// =============================================================================
// MAIN APPLICATION
// =============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "SSD1680 E-Paper Display Driver Test");
    ESP_LOGI(TAG, "WeAct Studio 2.13\" (250x122)");
    ESP_LOGI(TAG, "=================================================");

    // Configure display
    ssd1680_config_t config = {
        .pin_sck = PIN_SPI_SCK,
        .pin_mosi = PIN_SPI_MOSI,
        .pin_cs = PIN_EPD_CS,
        .pin_dc = PIN_EPD_DC,
        .pin_rst = PIN_EPD_RST,
        .pin_busy = PIN_EPD_BUSY,
        .spi_clock_speed_hz = SPI_CLOCK_HZ,
    };

    // Initialize display
    ssd1680_t display;
    if (!ssd1680_init(&display, &config)) {
        ESP_LOGE(TAG, "Failed to initialize SSD1680!");
        return;
    }

    ESP_LOGI(TAG, "Display initialized successfully!");
    ESP_LOGI(TAG, "");

    // =========================================================================
    // TEST 1: Clear Screen
    // =========================================================================
    ESP_LOGI(TAG, "TEST 1: Clearing screen to white");
    ssd1680_clear_screen(&display);

    ESP_LOGI(TAG, "Screen should now be completely white.");
    ESP_LOGI(TAG, "Waiting 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // =========================================================================
    // TEST 2: Simple Shapes
    // =========================================================================
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "TEST 2: Drawing test pattern");

    draw_test_pattern(&display);
    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "You should see:");
    ESP_LOGI(TAG, "  - Border around screen");
    ESP_LOGI(TAG, "  - Filled square in center");
    ESP_LOGI(TAG, "  - Outline square around it");
    ESP_LOGI(TAG, "  - Numbers 1,2,3,0 in corners");
    ESP_LOGI(TAG, "  - Cross through center");
    ESP_LOGI(TAG, "Waiting 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 3: Checkerboard
    // =========================================================================
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "TEST 3: Drawing checkerboard pattern");

    draw_checkerboard(&display, 16);
    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "You should see a checkerboard (16x16 pixel squares)");
    ESP_LOGI(TAG, "Waiting 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 4: Fine Checkerboard
    // =========================================================================
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "TEST 4: Drawing fine checkerboard");

    draw_checkerboard(&display, 4);
    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "You should see a fine checkerboard (4x4 pixel squares)");
    ESP_LOGI(TAG, "Waiting 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =========================================================================
    // TEST 5: All Black
    // =========================================================================
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "TEST 5: Filling screen black");

    memset(display.framebuffer, 0x00, SSD1680_BUFFER_SIZE);  // All black
    ssd1680_display_frame(&display);

    ESP_LOGI(TAG, "Screen should be completely black");
    ESP_LOGI(TAG, "Waiting 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // =========================================================================
    // TEST 6: Final Clear
    // =========================================================================
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "TEST 6: Final clear to white");

    ssd1680_clear_screen(&display);

    ESP_LOGI(TAG, "Screen should be white again");

    // =========================================================================
    // Enter Sleep Mode
    // =========================================================================
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Entering sleep mode to save power");
    ssd1680_sleep(&display);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "All tests complete!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "If you saw all the patterns correctly, your");
    ESP_LOGI(TAG, "SSD1680 driver is working perfectly!");
    ESP_LOGI(TAG, "=================================================");

    // Keep running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
