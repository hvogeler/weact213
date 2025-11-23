#ifndef SSD1675_LOWLEVEL_H
#define SSD1675_LOWLEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

/**
 * @brief SSD1675 E-Paper Display Low-Level Driver
 *
 * This driver provides direct hardware control for the SSD1675 e-paper controller.
 * Perfect for learning how e-paper displays work at the hardware level!
 *
 * WeAct Studio 2.13" Specs:
 * - Resolution: 250 x 122 pixels
 * - Controller: SSD1675
 * - Colors: Black and White
 * - Interface: SPI
 */

// =============================================================================
// DISPLAY SPECIFICATIONS
// =============================================================================

#define SSD1675_WIDTH       250     // Display width in pixels
#define SSD1675_HEIGHT      122     // Display height in pixels
#define SSD1675_BUFFER_SIZE ((SSD1675_WIDTH * SSD1675_HEIGHT) / 8)  // Bytes needed

// =============================================================================
// SSD1675 COMMAND DEFINITIONS
// =============================================================================
// These are the control codes the SSD1675 understands

#define SSD1675_CMD_DRIVER_OUTPUT_CONTROL       0x01  // Set MUX, scanning direction
#define SSD1675_CMD_BOOSTER_SOFT_START          0x0C  // Set booster strength
#define SSD1675_CMD_GATE_SCAN_START             0x0F  // Set gate scan start position
#define SSD1675_CMD_DEEP_SLEEP_MODE             0x10  // Enter deep sleep
#define SSD1675_CMD_DATA_ENTRY_MODE             0x11  // Set data entry sequence
#define SSD1675_CMD_SW_RESET                    0x12  // Software reset
#define SSD1675_CMD_TEMP_SENSOR_CONTROL         0x1A  // Temperature sensor control
#define SSD1675_CMD_MASTER_ACTIVATION           0x20  // Activate display update
#define SSD1675_CMD_DISPLAY_UPDATE_CONTROL_1    0x21  // Display update control option
#define SSD1675_CMD_DISPLAY_UPDATE_CONTROL_2    0x22  // Display update sequence
#define SSD1675_CMD_WRITE_RAM                   0x24  // Write to BW RAM
#define SSD1675_CMD_WRITE_VCOM_REGISTER         0x2C  // Write VCOM register
#define SSD1675_CMD_WRITE_LUT_REGISTER          0x32  // Write LUT register
#define SSD1675_CMD_SET_DUMMY_LINE_PERIOD       0x3A  // Set dummy line period
#define SSD1675_CMD_SET_GATE_TIME               0x3B  // Set gate line width
#define SSD1675_CMD_BORDER_WAVEFORM_CONTROL     0x3C  // Border waveform control
#define SSD1675_CMD_SET_RAM_X_ADDRESS_RANGE     0x44  // Set RAM X address start/end
#define SSD1675_CMD_SET_RAM_Y_ADDRESS_RANGE     0x45  // Set RAM Y address start/end
#define SSD1675_CMD_SET_RAM_X_ADDRESS_COUNTER   0x4E  // Set RAM X address counter
#define SSD1675_CMD_SET_RAM_Y_ADDRESS_COUNTER   0x4F  // Set RAM Y address counter
#define SSD1675_CMD_TERMINATE_FRAME_READ_WRITE  0xFF  // End frame read/write (NOP)

// =============================================================================
// CONFIGURATION STRUCTURE
// =============================================================================

/**
 * @brief Pin configuration for SSD1675
 */
typedef struct {
    gpio_num_t pin_sck;     // SPI Clock
    gpio_num_t pin_mosi;    // SPI MOSI (Master Out Slave In)
    gpio_num_t pin_cs;      // Chip Select (active LOW)
    gpio_num_t pin_dc;      // Data/Command select (LOW=cmd, HIGH=data)
    gpio_num_t pin_rst;     // Reset (active LOW)
    gpio_num_t pin_busy;    // Busy signal (HIGH=busy)
    int spi_clock_speed_hz; // SPI clock speed (typically 4-20 MHz)
} ssd1675_config_t;

/**
 * @brief SSD1675 device handle
 */
typedef struct {
    spi_device_handle_t spi;
    ssd1675_config_t config;
    uint8_t *framebuffer;   // Pointer to framebuffer in memory
} ssd1675_t;

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

/**
 * @brief Initialize the SSD1675 display
 *
 * This function:
 * 1. Configures GPIO pins
 * 2. Initializes SPI bus
 * 3. Resets the display
 * 4. Sends initialization sequence
 *
 * @param dev Pointer to device handle
 * @param config Pointer to pin configuration
 * @return true on success, false on failure
 */
bool ssd1675_init(ssd1675_t *dev, const ssd1675_config_t *config);

/**
 * @brief Send a command to SSD1675
 *
 * @param dev Device handle
 * @param cmd Command byte (see SSD1675_CMD_* defines)
 */
void ssd1675_send_command(ssd1675_t *dev, uint8_t cmd);

/**
 * @brief Send data to SSD1675
 *
 * @param dev Device handle
 * @param data Pointer to data buffer
 * @param len Number of bytes to send
 */
void ssd1675_send_data(ssd1675_t *dev, const uint8_t *data, size_t len);

/**
 * @brief Send a single data byte
 *
 * @param dev Device handle
 * @param data Data byte
 */
void ssd1675_send_data_byte(ssd1675_t *dev, uint8_t data);

/**
 * @brief Wait until display is not busy
 *
 * The BUSY pin goes HIGH when the display is updating.
 * This function blocks until BUSY goes LOW.
 *
 * @param dev Device handle
 */
void ssd1675_wait_until_idle(ssd1675_t *dev);

/**
 * @brief Hardware reset the display
 *
 * Toggles the RST pin: LOW -> HIGH -> LOW -> HIGH
 *
 * @param dev Device handle
 */
void ssd1675_reset(ssd1675_t *dev);

/**
 * @brief Clear the entire display (all white)
 *
 * @param dev Device handle
 */
void ssd1675_clear_screen(ssd1675_t *dev);

/**
 * @brief Draw a pixel in the framebuffer
 *
 * NOTE: This only updates the framebuffer in RAM.
 * You must call ssd1675_display_frame() to show it on screen.
 *
 * @param dev Device handle
 * @param x X coordinate (0 to SSD1675_WIDTH-1)
 * @param y Y coordinate (0 to SSD1675_HEIGHT-1)
 * @param color 1 = BLACK, 0 = WHITE
 */
void ssd1675_draw_pixel(ssd1675_t *dev, int x, int y, uint8_t color);

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
void ssd1675_draw_rectangle(ssd1675_t *dev, int x0, int y0, int x1, int y1, bool filled);

/**
 * @brief Send the framebuffer to the display and refresh
 *
 * This is the key function that makes your drawing visible!
 *
 * @param dev Device handle
 */
void ssd1675_display_frame(ssd1675_t *dev);

/**
 * @brief Enter deep sleep mode (low power)
 *
 * @param dev Device handle
 */
void ssd1675_sleep(ssd1675_t *dev);

#endif // SSD1675_LOWLEVEL_H
