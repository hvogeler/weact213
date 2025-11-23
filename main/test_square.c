/**
 * @file test_square.c
 * @brief Simple test program to draw a square on SSD1675 e-paper
 *
 * This program demonstrates:
 * 1. Initializing the SSD1675 display
 * 2. Drawing pixels and shapes
 * 3. Updating the e-paper display
 *
 * Expected result: You'll see a square and some text patterns on your e-paper!
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1675_lowlevel.h"

static const char *TAG = "TEST";

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
// Update these to match your wiring!

#define PIN_SPI_SCK     6
#define PIN_SPI_MOSI    7
#define PIN_EPD_CS      10
#define PIN_EPD_DC      9
#define PIN_EPD_RST     4
#define PIN_EPD_BUSY    18

#define SPI_CLOCK_HZ    (4 * 1000 * 1000)  // 4 MHz - safe speed for e-paper

// =============================================================================
// HELPER FUNCTION: Draw a checkerboard pattern
// =============================================================================
void draw_checkerboard(ssd1675_t *dev, int square_size)
{
    ESP_LOGI(TAG, "Drawing checkerboard pattern");

    for (int y = 0; y < SSD1675_HEIGHT; y++) {
        for (int x = 0; x < SSD1675_WIDTH; x++) {
            // Determine if we're in a "black" square or "white" square
            int grid_x = x / square_size;
            int grid_y = y / square_size;
            bool is_black = ((grid_x + grid_y) % 2) == 0;

            ssd1675_draw_pixel(dev, x, y, is_black ? 1 : 0);
        }
    }
}

// =============================================================================
// HELPER FUNCTION: Draw a simple "5x7" digit
// =============================================================================
// This draws large digits for demonstration
void draw_large_digit(ssd1675_t *dev, int start_x, int start_y, int digit, int scale)
{
    // Simple 5x7 font patterns for digits 0-3
    // 1 = pixel on, 0 = pixel off
    const uint8_t font_5x7[][7] = {
        // Digit 0
        {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110},
        // Digit 1
        {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
        // Digit 2
        {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111},
        // Digit 3
        {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110},
    };

    if (digit < 0 || digit > 3) return;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (font_5x7[digit][row] & (1 << (4 - col))) {
                // Draw scaled pixel
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        ssd1675_draw_pixel(dev,
                                         start_x + col * scale + sx,
                                         start_y + row * scale + sy,
                                         1);  // Black
                    }
                }
            }
        }
    }
}

// =============================================================================
// MAIN APPLICATION
// =============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "SSD1675 E-Paper Low-Level Driver Test");
    ESP_LOGI(TAG, "=================================================");

    // -------------------------------------------------------------------------
    // STEP 1: Configure the display
    // -------------------------------------------------------------------------
    ssd1675_config_t config = {
        .pin_sck = PIN_SPI_SCK,
        .pin_mosi = PIN_SPI_MOSI,
        .pin_cs = PIN_EPD_CS,
        .pin_dc = PIN_EPD_DC,
        .pin_rst = PIN_EPD_RST,
        .pin_busy = PIN_EPD_BUSY,
        .spi_clock_speed_hz = SPI_CLOCK_HZ,
    };

    // -------------------------------------------------------------------------
    // STEP 2: Initialize the device
    // -------------------------------------------------------------------------
    ssd1675_t display;

    if (!ssd1675_init(&display, &config)) {
        ESP_LOGE(TAG, "Failed to initialize display!");
        return;
    }

    ESP_LOGI(TAG, "Display initialized successfully!");

    // -------------------------------------------------------------------------
    // STEP 3: Clear the screen first
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Clearing screen...");
    ssd1675_clear_screen(&display);

    ESP_LOGI(TAG, "Waiting 3 seconds before drawing...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // -------------------------------------------------------------------------
    // STEP 4: Draw some shapes!
    // -------------------------------------------------------------------------

    // Draw a border around the entire screen
    ESP_LOGI(TAG, "Drawing border");
    ssd1675_draw_rectangle(&display, 0, 0, SSD1675_WIDTH - 1, SSD1675_HEIGHT - 1, false);

    // Draw a filled square in the center
    ESP_LOGI(TAG, "Drawing filled square");
    int square_size = 60;
    int center_x = SSD1675_WIDTH / 2;
    int center_y = SSD1675_HEIGHT / 2;
    ssd1675_draw_rectangle(&display,
                          center_x - square_size/2,
                          center_y - square_size/2,
                          center_x + square_size/2,
                          center_y + square_size/2,
                          true);  // filled

    // Draw an outline square around it
    ESP_LOGI(TAG, "Drawing outline square");
    ssd1675_draw_rectangle(&display,
                          center_x - square_size/2 - 10,
                          center_y - square_size/2 - 10,
                          center_x + square_size/2 + 10,
                          center_y + square_size/2 + 10,
                          false);  // outline only

    // Draw some large numbers in the corners
    ESP_LOGI(TAG, "Drawing corner labels");
    draw_large_digit(&display, 10, 10, 1, 3);                          // Top-left: "1"
    draw_large_digit(&display, SSD1675_WIDTH - 30, 10, 2, 3);         // Top-right: "2"
    draw_large_digit(&display, 10, SSD1675_HEIGHT - 30, 3, 3);        // Bottom-left: "3"
    draw_large_digit(&display, SSD1675_WIDTH - 30, SSD1675_HEIGHT - 30, 0, 3);  // Bottom-right: "0"

    // -------------------------------------------------------------------------
    // STEP 5: Display the frame!
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Updating display...");
    ssd1675_display_frame(&display);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Test complete! Check your e-paper display.");
    ESP_LOGI(TAG, "You should see:");
    ESP_LOGI(TAG, "  - Border around the screen");
    ESP_LOGI(TAG, "  - Filled square in center");
    ESP_LOGI(TAG, "  - Outline square around it");
    ESP_LOGI(TAG, "  - Numbers 1,2,3,0 in corners");
    ESP_LOGI(TAG, "=================================================");

    // -------------------------------------------------------------------------
    // STEP 6: Wait and demonstrate animation (optional)
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Waiting 5 seconds before next demo...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Draw a checkerboard pattern
    ESP_LOGI(TAG, "Drawing checkerboard...");
    draw_checkerboard(&display, 16);  // 16x16 pixel squares
    ssd1675_display_frame(&display);

    ESP_LOGI(TAG, "Waiting 5 seconds before clearing...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Clear one more time
    ESP_LOGI(TAG, "Final clear");
    ssd1675_clear_screen(&display);

    // -------------------------------------------------------------------------
    // STEP 7: Enter sleep mode to save power
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Entering sleep mode");
    ssd1675_sleep(&display);

    ESP_LOGI(TAG, "Demo finished! System will continue running.");

    // Keep the program running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
