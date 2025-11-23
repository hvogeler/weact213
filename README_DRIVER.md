# SSD1680 E-Paper Display Driver

Complete low-level driver for the WeAct Studio 2.13" e-paper display with SSD1680 controller.

## 🎉 Status: Working Perfectly!

This driver has been tested and verified to work correctly with:
- **Display**: WeAct Studio 2.13" E-Paper
- **Controller**: SSD1680
- **Resolution**: 122 × 250 pixels (portrait)
- **Interface**: 4-wire SPI

---

## Quick Start

### 1. Build and Flash

```bash
cd /Users/hvo/Esp32/epaper
./build.sh      # Build the project
./flash.sh      # Flash to ESP32
```

### 2. Wiring

Connect your display to ESP32-S3:

| Display Pin | ESP32-S3 Pin | Function |
|-------------|--------------|----------|
| SCK | GPIO 6 | SPI Clock |
| MOSI | GPIO 7 | SPI Data Out |
| CS | GPIO 10 | Chip Select |
| DC | GPIO 9 | Data/Command |
| RST | GPIO 4 | Reset |
| BUSY | GPIO 18 | Busy Status |
| VCC | 3.3V | Power |
| GND | GND | Ground |

### 3. Basic Usage

```c
#include "ssd1680_lowlevel.h"

// Configure
ssd1680_config_t config = {
    .pin_sck = 6,
    .pin_mosi = 7,
    .pin_cs = 10,
    .pin_dc = 9,
    .pin_rst = 4,
    .pin_busy = 18,
    .spi_clock_speed_hz = 4000000,  // 4 MHz
};

// Initialize
ssd1680_t display;
ssd1680_init(&display, &config);

// Draw
ssd1680_draw_rectangle(&display, 10, 10, 50, 50, true);  // Filled square
ssd1680_display_frame(&display);  // Update display

// Sleep
ssd1680_sleep(&display);
```

---

## Features

✅ **Complete SSD1680 support**
- Hardware SPI communication
- Full display initialization
- Proper orientation handling
- Byte-aligned framebuffer

✅ **Drawing functions**
- Individual pixels
- Rectangles (filled or outline)
- Direct framebuffer access

✅ **Display operations**
- Clear screen (complete white)
- Full refresh
- Power management (deep sleep)

✅ **Robust implementation**
- DMA-capable memory allocation
- Timeout handling for BUSY pin
- Proper byte alignment (16 bytes/row)

---

## API Reference

See [QUICK_REFERENCE.md](doc/QUICK_REFERENCE.md) for complete API documentation.

### Core Functions

```c
// Initialization
bool ssd1680_init(ssd1680_t *dev, const ssd1680_config_t *config);

// Drawing
void ssd1680_draw_pixel(ssd1680_t *dev, int x, int y, uint8_t color);
void ssd1680_draw_rectangle(ssd1680_t *dev, int x0, int y0, int x1, int y1, bool filled);

// Display
void ssd1680_clear_screen(ssd1680_t *dev);
void ssd1680_display_frame(ssd1680_t *dev);
void ssd1680_sleep(ssd1680_t *dev);
```

---

## Technical Details

### Display Configuration

- **Orientation**: Portrait (122 wide × 250 tall)
- **Framebuffer**: 4,000 bytes (16-byte aligned rows)
- **Data Entry Mode**: 0x03 (X direction, X+, Y+)
- **MUX Gates**: 250 (0xF9)
- **RAM X Range**: 0-15 bytes (0x00 to 0x0F)
- **RAM Y Range**: 0-249 pixels (0x00 to 0xF9)

### Why 16-Byte Alignment?

The display is 122 pixels wide:
- 122 ÷ 8 = **15.25 bytes per row**
- Rounded up to **16 bytes** for proper alignment
- This prevents the "parallelogram" distortion effect

### Memory Layout

```
Row 0:   [16 bytes]  122 pixels + 6 padding bits
Row 1:   [16 bytes]  122 pixels + 6 padding bits
...
Row 249: [16 bytes]  122 pixels + 6 padding bits
Total:   4,000 bytes
```

---

## Documentation

- **[FINAL_CONFIGURATION.md](doc/FINAL_CONFIGURATION.md)** - Complete configuration details
- **[QUICK_REFERENCE.md](doc/QUICK_REFERENCE.md)** - API reference
- **[SSD1680_vs_SSD1675.md](doc/SSD1680_vs_SSD1675.md)** - Controller differences
- **[UNDERSTANDING_SSD1675.md](doc/UNDERSTANDING_SSD1675.md)** - Low-level concepts

---

## Testing

Run the included test program:

```bash
# The test_ssd1680.c program runs 6 comprehensive tests:
# 1. Clear to white
# 2. Draw test patterns (border, squares, cross, numbers)
# 3. Checkerboard (16×16 pixels)
# 4. Fine checkerboard (4×4 pixels)
# 5. All black
# 6. Final clear
```

All tests should pass with clean, undistorted graphics.

---

## Troubleshooting

### Display shows garbage
- Check all pin connections
- Verify SPI clock ≤ 20 MHz
- Ensure BUSY pin is connected

### Shapes are distorted
- Verify `SSD1680_WIDTH_BYTES = 16` in header
- Check data entry mode is 0x03
- Confirm alignment in draw_pixel function

### Screen doesn't clear
- Ensure both BW and RED RAM are cleared
- Check BUSY pin functionality
- Verify display update control is 0xF7

---

## Next Steps: LVGL Integration

This low-level driver is ready for LVGL 9 integration!

The LVGL driver will:
1. Convert RGB colors to monochrome (brightness threshold)
2. Copy LVGL framebuffer to aligned e-paper buffer
3. Handle display refresh synchronization
4. Support rotation and mirroring

When you're ready to add LVGL support, we can build the display driver callbacks together!

---

## License

This code is provided as educational material for working with e-paper displays and ESP32.

---

## Credits

- Driver developed for WeAct Studio 2.13" E-Paper Display
- Based on SSD1680 datasheet specifications
- Built with ESP-IDF framework
- Designed for integration with LVGL graphics library

---

## Changelog

### v1.0 - Final Working Version
- ✅ Portrait orientation (122×250)
- ✅ 16-byte row alignment
- ✅ Complete initialization sequence
- ✅ Proper SSD1680 command set
- ✅ Both BW and RED RAM clearing
- ✅ All tests passing perfectly

### Previous Iterations
- v0.3: Fixed X address range
- v0.2: Swapped width/height for portrait
- v0.1: Initial SSD1675 attempt (incomplete)

---

**Status**: Production ready! ✅
