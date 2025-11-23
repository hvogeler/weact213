#ifndef SSD1680_LOWLEVEL_H
#define SSD1680_LOWLEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

/**
 * @brief SSD1680 E-Paper Display Low-Level Driver
 *
 * SSD1680 is a newer controller (successor to SSD1675) commonly found in:
 * - WeAct Studio 2.13" displays
 * - Waveshare 2.13" displays
 * - Other 250x122 e-paper modules
 *
 * Key differences from SSD1675:
 * - Different initialization sequence
 * - Updated command set
 * - Improved power management
 * - Better temperature compensation
 */

// =============================================================================
// DISPLAY SPECIFICATIONS
// =============================================================================
// WeAct 2.13" Display in PORTRAIT orientation
// Physical dimensions: 122 pixels wide × 250 pixels tall

#define SSD1680_WIDTH       122     // Display width in pixels (portrait)
#define SSD1680_HEIGHT      250     // Display height in pixels (portrait)
#define SSD1680_WIDTH_BYTES 16      // Bytes per row (round up 122/8 = 15.25 to 16)
#define SSD1680_BUFFER_SIZE (SSD1680_WIDTH_BYTES * SSD1680_HEIGHT)  // 16 * 250 = 4000 bytes

// =============================================================================
// SSD1680 COMMAND DEFINITIONS
// =============================================================================

#define SSD1680_CMD_DRIVER_OUTPUT_CONTROL           0x01
#define SSD1680_CMD_GATE_DRIVING_VOLTAGE            0x03
#define SSD1680_CMD_SOURCE_DRIVING_VOLTAGE          0x04
#define SSD1680_CMD_DEEP_SLEEP_MODE                 0x10
#define SSD1680_CMD_DATA_ENTRY_MODE                 0x11
#define SSD1680_CMD_SW_RESET                        0x12
#define SSD1680_CMD_TEMP_SENSOR_CONTROL             0x18
#define SSD1680_CMD_TEMP_SENSOR_WRITE               0x1A
#define SSD1680_CMD_MASTER_ACTIVATION               0x20
#define SSD1680_CMD_DISPLAY_UPDATE_CONTROL_1        0x21
#define SSD1680_CMD_DISPLAY_UPDATE_CONTROL_2        0x22
#define SSD1680_CMD_WRITE_RAM_BW                    0x24  // Write B/W RAM
#define SSD1680_CMD_WRITE_RAM_RED                   0x26  // Write RED RAM (if supported)
#define SSD1680_CMD_VCOM_SENSE                      0x28
#define SSD1680_CMD_VCOM_SENSE_DURATION             0x29
#define SSD1680_CMD_PROGRAM_VCOM_OTP                0x2A
#define SSD1680_CMD_WRITE_VCOM_REGISTER             0x2C
#define SSD1680_CMD_OTP_REGISTER_READ               0x2D
#define SSD1680_CMD_WRITE_LUT_REGISTER              0x32
#define SSD1680_CMD_DUMMY_LINE_PERIOD               0x3A
#define SSD1680_CMD_GATE_LINE_WIDTH                 0x3B
#define SSD1680_CMD_BORDER_WAVEFORM_CONTROL         0x3C
#define SSD1680_CMD_SET_RAM_X_ADDRESS_START_END     0x44
#define SSD1680_CMD_SET_RAM_Y_ADDRESS_START_END     0x45
#define SSD1680_CMD_AUTO_WRITE_RED_PATTERN          0x46
#define SSD1680_CMD_AUTO_WRITE_BW_PATTERN           0x47
#define SSD1680_CMD_SET_RAM_X_ADDRESS_COUNTER       0x4E
#define SSD1680_CMD_SET_RAM_Y_ADDRESS_COUNTER       0x4F
#define SSD1680_CMD_NOP                             0x7F

// =============================================================================
// CONFIGURATION STRUCTURE
// =============================================================================

/**
 * @brief Pin configuration for SSD1680
 */
typedef struct {
    gpio_num_t pin_sck;     // SPI Clock
    gpio_num_t pin_mosi;    // SPI MOSI (Master Out Slave In)
    gpio_num_t pin_cs;      // Chip Select (active LOW)
    gpio_num_t pin_dc;      // Data/Command select (LOW=cmd, HIGH=data)
    gpio_num_t pin_rst;     // Reset (active LOW)
    gpio_num_t pin_busy;    // Busy signal (HIGH=busy)
    int spi_clock_speed_hz; // SPI clock speed (typically 4-20 MHz)
} ssd1680_config_t;

/**
 * @brief SSD1680 device handle
 */
typedef struct {
    spi_device_handle_t spi;
    ssd1680_config_t config;
    uint8_t *framebuffer;   // Pointer to framebuffer in memory
} ssd1680_t;

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

/**
 * @brief Initialize the SSD1680 display
 *
 * @param dev Pointer to device handle
 * @param config Pointer to pin configuration
 * @return true on success, false on failure
 */
bool ssd1680_init(ssd1680_t *dev, const ssd1680_config_t *config);

/**
 * @brief Send a command to SSD1680
 *
 * @param dev Device handle
 * @param cmd Command byte
 */
void ssd1680_send_command(ssd1680_t *dev, uint8_t cmd);

/**
 * @brief Send data to SSD1680
 *
 * @param dev Device handle
 * @param data Pointer to data buffer
 * @param len Number of bytes to send
 */
void ssd1680_send_data(ssd1680_t *dev, const uint8_t *data, size_t len);

/**
 * @brief Send a single data byte
 *
 * @param dev Device handle
 * @param data Data byte
 */
void ssd1680_send_data_byte(ssd1680_t *dev, uint8_t data);

/**
 * @brief Wait until display is not busy
 *
 * @param dev Device handle
 */
void ssd1680_wait_until_idle(ssd1680_t *dev);

/**
 * @brief Hardware reset the display
 *
 * @param dev Device handle
 */
void ssd1680_reset(ssd1680_t *dev);

/**
 * @brief Clear the entire display (all white)
 *
 * @param dev Device handle
 */
void ssd1680_clear_screen(ssd1680_t *dev);

/**
 * @brief Draw a pixel in the framebuffer
 *
 * @param dev Device handle
 * @param x X coordinate (0 to SSD1680_WIDTH-1)
 * @param y Y coordinate (0 to SSD1680_HEIGHT-1)
 * @param color 1 = BLACK, 0 = WHITE
 */
void ssd1680_draw_pixel(ssd1680_t *dev, int x, int y, uint8_t color);

/**
 * @brief Draw a rectangle in the framebuffer
 *
 * @param dev Device handle
 * @param x0 Top-left X coordinate
 * @param y0 Top-left Y coordinate
 * @param x1 Bottom-right X coordinate
 * @param y1 Bottom-right Y coordinate
 * @param filled true = filled rectangle, false = outline only
 */
void ssd1680_draw_rectangle(ssd1680_t *dev, int x0, int y0, int x1, int y1, bool filled);

/**
 * @brief Send the framebuffer to the display and refresh
 *
 * @param dev Device handle
 */
void ssd1680_display_frame(ssd1680_t *dev);

/**
 * @brief Enter deep sleep mode (low power)
 *
 * @param dev Device handle
 */
void ssd1680_sleep(ssd1680_t *dev);

#endif // SSD1680_LOWLEVEL_H
