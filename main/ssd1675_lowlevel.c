#include "ssd1675_lowlevel.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "SSD1675";

// =============================================================================
// LUT (Look-Up Table) for Display Waveforms
// =============================================================================
/**
 * The LUT controls how the e-paper transitions between states.
 * It defines voltage levels and timing for pixel changes.
 *
 * This is a full refresh LUT optimized for the WeAct 2.13" display.
 * 30 bytes total: controls the waveform phases
 *
 * Understanding LUT (simplified):
 * - E-paper pixels use electrophoresis (charged particles in fluid)
 * - To change a pixel: apply voltage pulses in specific patterns
 * - LUT defines these patterns: voltage levels, phase durations, repetitions
 *
 * You usually don't need to modify this unless you want:
 * - Faster refresh (with more ghosting)
 * - Better quality (slower refresh)
 * - Partial refresh capability
 */
static const uint8_t ssd1675_lut_full_update[] = {
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,  // LUT0: Phase A
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,  // LUT1: Phase B
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,  // LUT2: Phase C
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,  // LUT3: Phase D
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LUT4: Phase E (unused)
    0x03, 0x03, 0x00, 0x00, 0x02,              // Frame timing
    0x09, 0x09, 0x00, 0x00, 0x02,              // More timing
    0x03, 0x03, 0x00, 0x00, 0x02,              // Gate timing
    0x00, 0x00, 0x00, 0x00, 0x00,              // Reserved
    0x00, 0x00, 0x00, 0x00, 0x00,              // Reserved
    0x00, 0x00, 0x00, 0x00, 0x00,              // Reserved
    0x00, 0x00, 0x00, 0x00, 0x00,              // Reserved
};

// =============================================================================
// LOW-LEVEL SPI COMMUNICATION FUNCTIONS
// =============================================================================

/**
 * @brief Send a command byte to SSD1675
 *
 * How it works:
 * 1. Set DC pin LOW (tells SSD1675: "this is a command")
 * 2. Transmit the command byte via SPI
 * 3. Wait for transmission to complete
 */
void ssd1675_send_command(ssd1675_t *dev, uint8_t cmd)
{
    spi_transaction_t trans = {
        .length = 8,           // 8 bits = 1 byte
        .tx_buffer = &cmd,     // Pointer to command byte
        .rx_buffer = NULL,     // We don't read anything back
    };

    // Set D/C pin LOW for command
    gpio_set_level(dev->config.pin_dc, 0);

    // Transmit via SPI (blocking call)
    ESP_ERROR_CHECK(spi_device_polling_transmit(dev->spi, &trans));
}

/**
 * @brief Send data bytes to SSD1675
 *
 * How it works:
 * 1. Set DC pin HIGH (tells SSD1675: "this is data")
 * 2. Transmit all data bytes via SPI
 */
void ssd1675_send_data(ssd1675_t *dev, const uint8_t *data, size_t len)
{
    if (len == 0) return;

    spi_transaction_t trans = {
        .length = len * 8,     // Length in bits
        .tx_buffer = data,
        .rx_buffer = NULL,
    };

    // Set D/C pin HIGH for data
    gpio_set_level(dev->config.pin_dc, 1);

    ESP_ERROR_CHECK(spi_device_polling_transmit(dev->spi, &trans));
}

/**
 * @brief Send a single data byte (convenience function)
 */
void ssd1675_send_data_byte(ssd1675_t *dev, uint8_t data)
{
    ssd1675_send_data(dev, &data, 1);
}

// =============================================================================
// CONTROL FUNCTIONS
// =============================================================================

/**
 * @brief Wait for the display to finish updating
 *
 * The BUSY pin tells us when the display is working:
 * - HIGH (1) = Display is busy (refreshing the screen)
 * - LOW (0) = Display is idle (ready for new commands)
 *
 * E-paper refresh can take 1-3 seconds!
 */
void ssd1675_wait_until_idle(ssd1675_t *dev)
{
    ESP_LOGI(TAG, "Waiting for display to be ready...");

    // Poll the BUSY pin until it goes LOW
    while (gpio_get_level(dev->config.pin_busy) == 1) {
        vTaskDelay(pdMS_TO_TICKS(10));  // Check every 10ms
    }

    ESP_LOGI(TAG, "Display ready!");
}

/**
 * @brief Hardware reset the display
 *
 * Reset sequence (from datasheet):
 * 1. RST = HIGH (normal operation)
 * 2. Wait 200ms
 * 3. RST = LOW (activate reset)
 * 4. Wait 10ms
 * 5. RST = HIGH (release reset)
 * 6. Wait 200ms for display to stabilize
 */
void ssd1675_reset(ssd1675_t *dev)
{
    ESP_LOGI(TAG, "Hardware reset");

    gpio_set_level(dev->config.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(dev->config.pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_level(dev->config.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
}

// =============================================================================
// INITIALIZATION
// =============================================================================

/**
 * @brief Initialize the SSD1675 display
 *
 * This is the most complex function - it sets up everything!
 */
bool ssd1675_init(ssd1675_t *dev, const ssd1675_config_t *config)
{
    ESP_LOGI(TAG, "Initializing SSD1675 (250x122 WeAct 2.13\")");

    // Copy configuration
    memcpy(&dev->config, config, sizeof(ssd1675_config_t));

    // -------------------------------------------------------------------------
    // STEP 1: Configure GPIO pins
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Configuring GPIO pins");

    // Configure DC, RST as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->pin_dc) | (1ULL << config->pin_rst),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Configure BUSY as input
    io_conf.pin_bit_mask = (1ULL << config->pin_busy);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // -------------------------------------------------------------------------
    // STEP 2: Initialize SPI bus
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Initializing SPI bus");

    spi_bus_config_t buscfg = {
        .mosi_io_num = config->pin_mosi,
        .miso_io_num = -1,              // No MISO (we don't read from display)
        .sclk_io_num = config->pin_sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SSD1675_BUFFER_SIZE,
    };

    // Initialize the SPI bus (using SPI2_HOST)
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // -------------------------------------------------------------------------
    // STEP 3: Add device to SPI bus
    // -------------------------------------------------------------------------
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = config->spi_clock_speed_hz,
        .mode = 0,                      // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = config->pin_cs, // CS pin
        .queue_size = 1,                // We only need one transaction at a time
        .flags = SPI_DEVICE_HALFDUPLEX, // We only send, never receive
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &dev->spi));

    // -------------------------------------------------------------------------
    // STEP 4: Allocate framebuffer in memory
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Allocating framebuffer (%d bytes)", SSD1675_BUFFER_SIZE);

    dev->framebuffer = heap_caps_malloc(SSD1675_BUFFER_SIZE, MALLOC_CAP_DMA);
    if (dev->framebuffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer!");
        return false;
    }

    // Initialize to all white (0xFF in e-paper means white)
    memset(dev->framebuffer, 0xFF, SSD1675_BUFFER_SIZE);

    // -------------------------------------------------------------------------
    // STEP 5: Hardware reset
    // -------------------------------------------------------------------------
    ssd1675_reset(dev);

    // -------------------------------------------------------------------------
    // STEP 6: Send initialization sequence to SSD1675
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Sending initialization sequence");

    // Software reset
    ssd1675_send_command(dev, SSD1675_CMD_SW_RESET);
    ssd1675_wait_until_idle(dev);

    // Driver Output Control
    // Sets the MUX gate lines (height) and scanning direction
    // Format: [A7-A0] [A8+B2-B0] [B7]
    // For 122 lines: MUX = 121 (0x79)
    ssd1675_send_command(dev, SSD1675_CMD_DRIVER_OUTPUT_CONTROL);
    ssd1675_send_data_byte(dev, 0x79);  // (122-1) & 0xFF
    ssd1675_send_data_byte(dev, 0x00);  // ((122-1) >> 8) & 0x01
    ssd1675_send_data_byte(dev, 0x00);  // GD=0, SM=0, TB=0

    // Data Entry Mode
    // Controls the direction data is written to RAM
    // 0x03 = X increment, Y increment (normal left-to-right, top-to-bottom)
    ssd1675_send_command(dev, SSD1675_CMD_DATA_ENTRY_MODE);
    ssd1675_send_data_byte(dev, 0x03);

    // Set RAM X address range (0 to 31)
    // X address is in bytes: 250 pixels / 8 bits = 31.25 bytes → 31 (0x1F)
    ssd1675_send_command(dev, SSD1675_CMD_SET_RAM_X_ADDRESS_RANGE);
    ssd1675_send_data_byte(dev, 0x00);  // Start address (0)
    ssd1675_send_data_byte(dev, 0x1F);  // End address (31)

    // Set RAM Y address range (0 to 121)
    ssd1675_send_command(dev, SSD1675_CMD_SET_RAM_Y_ADDRESS_RANGE);
    ssd1675_send_data_byte(dev, 0x00);  // Start address LOW
    ssd1675_send_data_byte(dev, 0x00);  // Start address HIGH
    ssd1675_send_data_byte(dev, 0x79);  // End address LOW (121)
    ssd1675_send_data_byte(dev, 0x00);  // End address HIGH

    // Border Waveform Control
    // 0x05 = Follow LUT1 (normal border behavior)
    ssd1675_send_command(dev, SSD1675_CMD_BORDER_WAVEFORM_CONTROL);
    ssd1675_send_data_byte(dev, 0x05);

    // Temperature Sensor Control
    // 0x80 = Use internal temperature sensor
    ssd1675_send_command(dev, SSD1675_CMD_TEMP_SENSOR_CONTROL);
    ssd1675_send_data_byte(dev, 0x80);

    // Write LUT register
    // Upload the waveform lookup table
    ssd1675_send_command(dev, SSD1675_CMD_WRITE_LUT_REGISTER);
    ssd1675_send_data(dev, ssd1675_lut_full_update, sizeof(ssd1675_lut_full_update));

    ESP_LOGI(TAG, "Initialization complete!");
    return true;
}

// =============================================================================
// DRAWING FUNCTIONS
// =============================================================================

/**
 * @brief Set or clear a pixel in the framebuffer
 *
 * Understanding the bit manipulation:
 * - Framebuffer is a 1D array of bytes
 * - Each byte represents 8 horizontal pixels
 * - Bit 7 (MSB) = leftmost pixel, Bit 0 (LSB) = rightmost pixel
 * - 0 = BLACK, 1 = WHITE (opposite of what you might expect!)
 *
 * Example: To draw pixel at (10, 5)
 * 1. Calculate byte position: (5 * 250 + 10) / 8 = 156.25 → byte 156
 * 2. Calculate bit position: 10 % 8 = 2 (bit 2 within the byte)
 * 3. Set or clear that bit
 */
void ssd1675_draw_pixel(ssd1675_t *dev, int x, int y, uint8_t color)
{
    // Bounds checking
    if (x < 0 || x >= SSD1675_WIDTH || y < 0 || y >= SSD1675_HEIGHT) {
        return;
    }

    // Calculate which byte this pixel is in
    int byte_index = (y * SSD1675_WIDTH + x) / 8;

    // Calculate which bit within that byte (0-7)
    int bit_index = x % 8;

    if (color == 0) {
        // WHITE: Set bit to 1
        dev->framebuffer[byte_index] |= (1 << (7 - bit_index));
    } else {
        // BLACK: Clear bit to 0
        dev->framebuffer[byte_index] &= ~(1 << (7 - bit_index));
    }
}

/**
 * @brief Draw a rectangle (filled or outline)
 */
void ssd1675_draw_rectangle(ssd1675_t *dev, int x0, int y0, int x1, int y1, bool filled)
{
    // Ensure x0 <= x1 and y0 <= y1
    if (x0 > x1) { int temp = x0; x0 = x1; x1 = temp; }
    if (y0 > y1) { int temp = y0; y0 = y1; y1 = temp; }

    if (filled) {
        // Fill entire rectangle
        for (int y = y0; y <= y1; y++) {
            for (int x = x0; x <= x1; x++) {
                ssd1675_draw_pixel(dev, x, y, 1);  // 1 = BLACK
            }
        }
    } else {
        // Draw outline only
        // Top and bottom edges
        for (int x = x0; x <= x1; x++) {
            ssd1675_draw_pixel(dev, x, y0, 1);
            ssd1675_draw_pixel(dev, x, y1, 1);
        }
        // Left and right edges
        for (int y = y0; y <= y1; y++) {
            ssd1675_draw_pixel(dev, x0, y, 1);
            ssd1675_draw_pixel(dev, x1, y, 1);
        }
    }
}

/**
 * @brief Clear the entire screen to white
 */
void ssd1675_clear_screen(ssd1675_t *dev)
{
    ESP_LOGI(TAG, "Clearing screen");

    // Fill framebuffer with 0xFF (white)
    memset(dev->framebuffer, 0xFF, SSD1675_BUFFER_SIZE);

    // Display the white framebuffer
    ssd1675_display_frame(dev);
}

/**
 * @brief Send framebuffer to display and trigger refresh
 *
 * This is where the magic happens!
 */
void ssd1675_display_frame(ssd1675_t *dev)
{
    ESP_LOGI(TAG, "Uploading framebuffer to display");

    // -------------------------------------------------------------------------
    // STEP 1: Set RAM X and Y cursors to start position (0, 0)
    // -------------------------------------------------------------------------
    ssd1675_send_command(dev, SSD1675_CMD_SET_RAM_X_ADDRESS_COUNTER);
    ssd1675_send_data_byte(dev, 0x00);

    ssd1675_send_command(dev, SSD1675_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    ssd1675_send_data_byte(dev, 0x00);  // Y LOW
    ssd1675_send_data_byte(dev, 0x00);  // Y HIGH

    // -------------------------------------------------------------------------
    // STEP 2: Write framebuffer to RAM
    // -------------------------------------------------------------------------
    ssd1675_send_command(dev, SSD1675_CMD_WRITE_RAM);
    ssd1675_send_data(dev, dev->framebuffer, SSD1675_BUFFER_SIZE);

    // -------------------------------------------------------------------------
    // STEP 3: Trigger display update
    // -------------------------------------------------------------------------
    // Display Update Control 2: Full refresh mode
    ssd1675_send_command(dev, SSD1675_CMD_DISPLAY_UPDATE_CONTROL_2);
    ssd1675_send_data_byte(dev, 0xF7);  // Full update sequence

    // Master Activation: Start the refresh!
    ssd1675_send_command(dev, SSD1675_CMD_MASTER_ACTIVATION);

    // Wait for refresh to complete (this takes a few seconds)
    ssd1675_wait_until_idle(dev);

    ESP_LOGI(TAG, "Display updated!");
}

/**
 * @brief Enter deep sleep mode
 */
void ssd1675_sleep(ssd1675_t *dev)
{
    ESP_LOGI(TAG, "Entering deep sleep mode");

    ssd1675_send_command(dev, SSD1675_CMD_DEEP_SLEEP_MODE);
    ssd1675_send_data_byte(dev, 0x01);  // Enter deep sleep
}
