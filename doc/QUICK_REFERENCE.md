# SSD1680 Driver Quick Reference

## ⚠️ Important: This is for the SSD1680 controller (not SSD1675)

If your WeAct 2.13" display was giving issues with the SSD1675 driver (half black, distortion), you need the SSD1680 driver!

## Building and Flashing

```bash
# Build the project
./build.sh

# Flash to device and monitor
./flash.sh

# Or manually:
. /Users/hvo/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.usbmodem* flash monitor
```

## Pin Configuration

Current pins (modify in [test_square.c](../main/test_square.c)):

```c
#define PIN_SPI_SCK     6
#define PIN_SPI_MOSI    7
#define PIN_EPD_CS      10
#define PIN_EPD_DC      9
#define PIN_EPD_RST     4
#define PIN_EPD_BUSY    18
```

## API Reference

### Initialization

```c
ssd1675_t display;
ssd1675_config_t config = {
    .pin_sck = 6,
    .pin_mosi = 7,
    .pin_cs = 10,
    .pin_dc = 9,
    .pin_rst = 4,
    .pin_busy = 18,
    .spi_clock_speed_hz = 4000000,  // 4 MHz
};

if (!ssd1675_init(&display, &config)) {
    // Handle error
}
```

### Drawing Functions

```c
// Draw a single pixel (0=white, 1=black)
ssd1675_draw_pixel(&display, x, y, 1);

// Draw a rectangle (outline or filled)
ssd1675_draw_rectangle(&display, x0, y0, x1, y1, true);  // filled
ssd1675_draw_rectangle(&display, x0, y0, x1, y1, false); // outline

// Clear entire screen to white
ssd1675_clear_screen(&display);
```

### Display Update

```c
// After drawing, update the display
ssd1675_display_frame(&display);
// This takes 1-3 seconds - be patient!
```

### Low-Level Commands

```c
// Send a command
ssd1675_send_command(&display, SSD1675_CMD_SW_RESET);

// Send data
uint8_t data[] = {0x01, 0x02, 0x03};
ssd1675_send_data(&display, data, 3);

// Wait for display to be ready
ssd1675_wait_until_idle(&display);

// Hardware reset
ssd1675_reset(&display);
```

### Power Management

```c
// Enter deep sleep mode (saves power)
ssd1675_sleep(&display);

// To wake up, reset the display
ssd1675_reset(&display);
```

## Common Commands Reference

| Command | Hex | Description |
|---------|-----|-------------|
| SW_RESET | 0x12 | Software reset |
| DRIVER_OUTPUT_CONTROL | 0x01 | Set resolution (MUX) |
| DATA_ENTRY_MODE | 0x11 | Set data direction |
| SET_RAM_X_ADDRESS_RANGE | 0x44 | Define X range |
| SET_RAM_Y_ADDRESS_RANGE | 0x45 | Define Y range |
| SET_RAM_X_ADDRESS_COUNTER | 0x4E | Set X cursor |
| SET_RAM_Y_ADDRESS_COUNTER | 0x4F | Set Y cursor |
| WRITE_RAM | 0x24 | Write framebuffer |
| DISPLAY_UPDATE_CONTROL_2 | 0x22 | Update mode |
| MASTER_ACTIVATION | 0x20 | Start refresh |
| WRITE_LUT_REGISTER | 0x32 | Upload LUT |
| DEEP_SLEEP_MODE | 0x10 | Enter sleep |

## Display Specifications

- **Resolution:** 122 × 250 pixels (portrait orientation)
- **Framebuffer size:** 4,000 bytes (16 × 250)
- **Bits per pixel:** 1 (monochrome)
- **Refresh time:** 1-3 seconds
- **SPI max speed:** 20 MHz (tested at 4 MHz)

## Memory Calculations

```
Physical:    122 × 250 = 30,500 pixels
Aligned:     16 bytes × 250 rows = 4,000 bytes
Padding:     6 bits per row × 250 = 1,500 padding bits

X addresses: 16 bytes → 0 to 15 (0x0F)
Y addresses: 250 rows → 0 to 249 (0xF9)
```

## Bit Manipulation Cheat Sheet

### Set a pixel to BLACK (0)
```c
framebuffer[byte_index] &= ~(1 << (7 - bit_index));
```

### Set a pixel to WHITE (1)
```c
framebuffer[byte_index] |= (1 << (7 - bit_index));
```

### Toggle a pixel
```c
framebuffer[byte_index] ^= (1 << (7 - bit_index));
```

### Read a pixel
```c
uint8_t pixel = (framebuffer[byte_index] >> (7 - bit_index)) & 1;
```

## Troubleshooting

### Display not responding
1. Check wiring (especially CS, DC, RST)
2. Verify BUSY pin is connected
3. Try lower SPI speed (2 MHz)
4. Add delay after reset

### Inverted colors
The driver uses: 0 = BLACK, 1 = WHITE (in framebuffer)
Flip your logic if needed.

### Partial updates
This driver does FULL refresh only. For partial updates:
1. Modify LUT for partial refresh
2. Change update control to 0xC7 instead of 0xF7

### Ghosting
- Use full refresh LUT (already default)
- Clear screen between different images
- Avoid partial updates

### Slow refresh
E-paper is inherently slow (1-3 seconds). This is normal!
You can:
- Use faster LUT (more ghosting)
- Accept the slow refresh
- Use partial refresh for small changes

## Next Steps

1. ✅ You've built the low-level driver
2. ⏭️ Next: Create LVGL 9 display driver
3. 📚 Read: [UNDERSTANDING_SSD1675.md](UNDERSTANDING_SSD1675.md)

## Resources

- SSD1675 Datasheet: [Google "SSD1675 datasheet"]
- ESP-IDF SPI Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html
- LVGL Documentation: https://docs.lvgl.io/
