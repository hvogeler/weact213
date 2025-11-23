/**
 * @file ssd1680_lowlevel_swapped.c
 * @brief SSD1680 driver with swapped X/Y for WeAct 2.13" portrait orientation
 *
 * This version treats the display as 122x250 (portrait) instead of 250x122 (landscape)
 * Use this if your display shows content rotated 90 degrees or half-filled issues
 */

#include "ssd1680_lowlevel.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "SSD1680";

// =============================================================================
// SAME AS ORIGINAL - Communication functions
// =============================================================================

void ssd1680_send_command(ssd1680_t *dev, uint8_t cmd)
{
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
        .rx_buffer = NULL,
    };

    gpio_set_level(dev->config.pin_dc, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(dev->spi, &trans));
}

void ssd1680_send_data(ssd1680_t *dev, const uint8_t *data, size_t len)
{
    if (len == 0) return;

    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
        .rx_buffer = NULL,
    };

    gpio_set_level(dev->config.pin_dc, 1);
    ESP_ERROR_CHECK(spi_device_polling_transmit(dev->spi, &trans));
}

void ssd1680_send_data_byte(ssd1680_t *dev, uint8_t data)
{
    ssd1680_send_data(dev, &data, 1);
}

void ssd1680_wait_until_idle(ssd1680_t *dev)
{
    ESP_LOGI(TAG, "Waiting for display...");

    uint32_t timeout = 0;
    const uint32_t MAX_TIMEOUT_MS = 5000;

    while (gpio_get_level(dev->config.pin_busy) == 1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout += 10;

        if (timeout > MAX_TIMEOUT_MS) {
            ESP_LOGW(TAG, "Display busy timeout!");
            break;
        }
    }

    ESP_LOGI(TAG, "Display ready (waited %lu ms)", timeout);
}

void ssd1680_reset(ssd1680_t *dev)
{
    ESP_LOGI(TAG, "Hardware reset");

    gpio_set_level(dev->config.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(dev->config.pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(2));

    gpio_set_level(dev->config.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
}

// =============================================================================
// MODIFIED INITIALIZATION - Swapped dimensions
// =============================================================================

bool ssd1680_init(ssd1680_t *dev, const ssd1680_config_t *config)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Initializing SSD1680 (250x122, SWAPPED MODE)");
    ESP_LOGI(TAG, "Portrait orientation: 122 wide × 250 tall");
    ESP_LOGI(TAG, "=================================================");

    memcpy(&dev->config, config, sizeof(ssd1680_config_t));

    // GPIO Configuration
    ESP_LOGI(TAG, "Configuring GPIO");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->pin_dc) | (1ULL << config->pin_rst),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = (1ULL << config->pin_busy);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // SPI Configuration
    ESP_LOGI(TAG, "Configuring SPI");

    spi_bus_config_t buscfg = {
        .mosi_io_num = config->pin_mosi,
        .miso_io_num = -1,
        .sclk_io_num = config->pin_sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SSD1680_BUFFER_SIZE,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = config->spi_clock_speed_hz,
        .mode = 0,
        .spics_io_num = config->pin_cs,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &dev->spi));

    // Framebuffer
    ESP_LOGI(TAG, "Allocating framebuffer (%d bytes)", SSD1680_BUFFER_SIZE);

    dev->framebuffer = heap_caps_malloc(SSD1680_BUFFER_SIZE, MALLOC_CAP_DMA);
    if (dev->framebuffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer!");
        return false;
    }

    memset(dev->framebuffer, 0xFF, SSD1680_BUFFER_SIZE);

    // Reset
    ssd1680_reset(dev);
    ssd1680_wait_until_idle(dev);

    // -------------------------------------------------------------------------
    // MODIFIED: Configuration for swapped orientation
    // -------------------------------------------------------------------------
    ESP_LOGI(TAG, "Sending init sequence (SWAPPED orientation)");

    // Software Reset
    ssd1680_send_command(dev, SSD1680_CMD_SW_RESET);
    ssd1680_wait_until_idle(dev);

    // Driver Output Control
    // For 250 height: MUX = 250-1 = 249 = 0xF9
    ssd1680_send_command(dev, SSD1680_CMD_DRIVER_OUTPUT_CONTROL);
    ssd1680_send_data_byte(dev, 0xF9);  // 250-1 LOW byte
    ssd1680_send_data_byte(dev, 0x00);  // HIGH byte
    ssd1680_send_data_byte(dev, 0x00);  // GD=0, SM=0, TB=0

    // Data Entry Mode
    // 0x03 = X increment, Y increment
    ssd1680_send_command(dev, SSD1680_CMD_DATA_ENTRY_MODE);
    ssd1680_send_data_byte(dev, 0x03);

    // Set RAM X address (now represents 122 pixels wide)
    // 122 / 8 = 15.25, so 0-15 (0x00 to 0x0F)
    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_X_ADDRESS_START_END);
    ssd1680_send_data_byte(dev, 0x00);  // Start
    ssd1680_send_data_byte(dev, 0x0F);  // End (15)

    // Set RAM Y address (now represents 250 pixels tall)
    // 0 to 249 (0xF9)
    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_Y_ADDRESS_START_END);
    ssd1680_send_data_byte(dev, 0x00);  // Start LOW
    ssd1680_send_data_byte(dev, 0x00);  // Start HIGH
    ssd1680_send_data_byte(dev, 0xF9);  // End LOW (249)
    ssd1680_send_data_byte(dev, 0x00);  // End HIGH

    // Border Waveform
    ssd1680_send_command(dev, SSD1680_CMD_BORDER_WAVEFORM_CONTROL);
    ssd1680_send_data_byte(dev, 0x05);

    // Display Update Control 1
    ssd1680_send_command(dev, SSD1680_CMD_DISPLAY_UPDATE_CONTROL_1);
    ssd1680_send_data_byte(dev, 0x00);
    ssd1680_send_data_byte(dev, 0x80);

    // Temperature Sensor
    ssd1680_send_command(dev, SSD1680_CMD_TEMP_SENSOR_CONTROL);
    ssd1680_send_data_byte(dev, 0x80);

    ESP_LOGI(TAG, "Init complete!");
    return true;
}

// =============================================================================
// MODIFIED DRAWING - Swapped X/Y coordinate mapping
// =============================================================================

void ssd1680_draw_pixel(ssd1680_t *dev, int x, int y, uint8_t color)
{
    // NOTE: With swapped orientation:
    // - User thinks: 250 wide, 122 tall
    // - Hardware is: 122 wide, 250 tall
    // So we need to map: user(x,y) -> hardware(y,x)

    // But since our buffer matches user expectation (250x122),
    // keep the same pixel function
    if (x < 0 || x >= SSD1680_WIDTH || y >= SSD1680_HEIGHT) {
        return;
    }

    int byte_index = (y * SSD1680_WIDTH + x) / 8;
    int bit_index = x % 8;

    if (color == 0) {
        dev->framebuffer[byte_index] |= (1 << (7 - bit_index));
    } else {
        dev->framebuffer[byte_index] &= ~(1 << (7 - bit_index));
    }
}

void ssd1680_draw_rectangle(ssd1680_t *dev, int x0, int y0, int x1, int y1, bool filled)
{
    if (x0 > x1) { int temp = x0; x0 = x1; x1 = temp; }
    if (y0 > y1) { int temp = y0; y0 = y1; y1 = temp; }

    if (filled) {
        for (int y = y0; y <= y1; y++) {
            for (int x = x0; x <= x1; x++) {
                ssd1680_draw_pixel(dev, x, y, 1);
            }
        }
    } else {
        for (int x = x0; x <= x1; x++) {
            ssd1680_draw_pixel(dev, x, y0, 1);
            ssd1680_draw_pixel(dev, x, y1, 1);
        }
        for (int y = y0; y <= y1; y++) {
            ssd1680_draw_pixel(dev, x0, y, 1);
            ssd1680_draw_pixel(dev, x1, y, 1);
        }
    }
}

void ssd1680_clear_screen(ssd1680_t *dev)
{
    ESP_LOGI(TAG, "Clearing screen");

    memset(dev->framebuffer, 0xFF, SSD1680_BUFFER_SIZE);

    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_X_ADDRESS_COUNTER);
    ssd1680_send_data_byte(dev, 0x00);

    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    ssd1680_send_data_byte(dev, 0x00);
    ssd1680_send_data_byte(dev, 0x00);

    ssd1680_send_command(dev, SSD1680_CMD_WRITE_RAM_BW);
    ssd1680_send_data(dev, dev->framebuffer, SSD1680_BUFFER_SIZE);

    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_X_ADDRESS_COUNTER);
    ssd1680_send_data_byte(dev, 0x00);
    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    ssd1680_send_data_byte(dev, 0x00);
    ssd1680_send_data_byte(dev, 0x00);

    ssd1680_send_command(dev, SSD1680_CMD_WRITE_RAM_RED);
    ssd1680_send_data(dev, dev->framebuffer, SSD1680_BUFFER_SIZE);

    ssd1680_send_command(dev, SSD1680_CMD_DISPLAY_UPDATE_CONTROL_2);
    ssd1680_send_data_byte(dev, 0xF7);

    ssd1680_send_command(dev, SSD1680_CMD_MASTER_ACTIVATION);

    ssd1680_wait_until_idle(dev);
}

void ssd1680_display_frame(ssd1680_t *dev)
{
    ESP_LOGI(TAG, "Uploading framebuffer");

    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_X_ADDRESS_COUNTER);
    ssd1680_send_data_byte(dev, 0x00);

    ssd1680_send_command(dev, SSD1680_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    ssd1680_send_data_byte(dev, 0x00);
    ssd1680_send_data_byte(dev, 0x00);

    ssd1680_send_command(dev, SSD1680_CMD_WRITE_RAM_BW);
    ssd1680_send_data(dev, dev->framebuffer, SSD1680_BUFFER_SIZE);

    ssd1680_send_command(dev, SSD1680_CMD_DISPLAY_UPDATE_CONTROL_2);
    ssd1680_send_data_byte(dev, 0xF7);

    ssd1680_send_command(dev, SSD1680_CMD_MASTER_ACTIVATION);

    ssd1680_wait_until_idle(dev);
}

void ssd1680_sleep(ssd1680_t *dev)
{
    ESP_LOGI(TAG, "Entering sleep");

    ssd1680_send_command(dev, SSD1680_CMD_DEEP_SLEEP_MODE);
    ssd1680_send_data_byte(dev, 0x01);

    vTaskDelay(pdMS_TO_TICKS(100));
}
