# SSD1680 Final Working Configuration

## ✅ Display Working Correctly!

This document describes the final working configuration for the WeAct 2.13" e-paper display with SSD1680 controller.

---

## Display Specifications

- **Controller**: SSD1680
- **Resolution**: 122 × 250 pixels (portrait orientation)
- **Physical Size**: 2.13 inches diagonal
- **Colors**: Black and White
- **Interface**: 4-wire SPI

---

## Critical Configuration Settings

### 1. Display Dimensions

```c
#define SSD1680_WIDTH       122     // Physical width in pixels
#define SSD1680_HEIGHT      250     // Physical height in pixels
#define SSD1680_WIDTH_BYTES 16      // Bytes per row (aligned)
#define SSD1680_BUFFER_SIZE 4000    // 16 * 250 bytes
```

**Why 16 bytes per row?**
- 122 pixels ÷ 8 bits/byte = 15.25 bytes
- Round up to 16 bytes for proper byte alignment
- This eliminates the "parallelogram" distortion

### 2. Driver Output Control

```c
// MUX Gate Lines = 250-1 = 249 = 0xF9
ssd1680_send_data_byte(dev, 0xF9);  // Height - 1
ssd1680_send_data_byte(dev, 0x00);  // HIGH byte
ssd1680_send_data_byte(dev, 0x00);  // GD=0, SM=0, TB=0
```

### 3. Data Entry Mode

```c
// 0x03 = X direction, X increment, Y increment
ssd1680_send_data_byte(dev, 0x03);
```

**What this means:**
- Data is written in X direction (horizontal rows)
- X increments left to right
- Y increments top to bottom

### 4. RAM Address Ranges

```c
// X address: 0-15 (16 bytes per row)
ssd1680_send_data_byte(dev, 0x00);  // X start
ssd1680_send_data_byte(dev, 0x0F);  // X end (15)

// Y address: 0-249 (250 rows)
ssd1680_send_data_byte(dev, 0x00);  // Y start LOW
ssd1680_send_data_byte(dev, 0x00);  // Y start HIGH
ssd1680_send_data_byte(dev, 0xF9);  // Y end LOW (249)
ssd1680_send_data_byte(dev, 0x00);  // Y end HIGH
```

### 5. Framebuffer Layout

```c
void ssd1680_draw_pixel(ssd1680_t *dev, int x, int y, uint8_t color)
{
    // Calculate byte position with aligned row width
    int byte_index = y * SSD1680_WIDTH_BYTES + (x / 8);
    int bit_index = x % 8;

    // ... set/clear bit ...
}
```

**Memory organization:**
```
Row 0:   [Byte 0][Byte 1]...[Byte 15]  (122 pixels + 6 padding bits)
Row 1:   [Byte 16][Byte 17]...[Byte 31]
...
Row 249: [Byte 3984][Byte 3985]...[Byte 3999]
```

---

## Issues That Were Fixed

### Issue 1: Half Black Screen ❌ → ✅
**Problem**: Only half the screen turned black when filling
**Cause**: Wrong orientation (250×122 instead of 122×250)
**Fix**: Swapped WIDTH and HEIGHT values

### Issue 2: Random Pixels on Right Edge ❌ → ✅
**Problem**: 3mm bar with random pixels on right edge
**Cause**: X address range too large (0-15 when 122 pixels only needed 0-14)
**Fix**: Initially tried 0-14, but final solution was 16-byte alignment

### Issue 3: Parallelogram Distortion ❌ → ✅
**Problem**: Squares appeared as parallelograms, first line issues
**Cause**: Non-aligned rows (15.25 bytes per row)
**Fix**: Aligned each row to 16 bytes

---

## Pin Configuration

```c
#define PIN_SPI_SCK     6
#define PIN_SPI_MOSI    7
#define PIN_EPD_CS      10
#define PIN_EPD_DC      9
#define PIN_EPD_RST     4
#define PIN_EPD_BUSY    18
#define SPI_CLOCK_HZ    4000000  // 4 MHz
```

---

## Memory Usage

| Component | Size | Notes |
|-----------|------|-------|
| Framebuffer | 4000 bytes | 16 × 250, DMA-capable |
| Actual pixels | 122 × 250 = 30,500 pixels | |
| Padding | 6 pixels × 250 rows = 1500 pixels | Unused padding bits |

---

## Performance Characteristics

- **Full refresh time**: ~2 seconds
- **SPI transfer**: ~10ms for full framebuffer
- **Power consumption (active)**: ~25mA
- **Power consumption (sleep)**: <1µA

---

## Code Structure

### Files

1. **[ssd1680_lowlevel.h](../main/ssd1680_lowlevel.h)** - API definitions
2. **[ssd1680_lowlevel.c](../main/ssd1680_lowlevel.c)** - Implementation
3. **[test_ssd1680.c](../main/test_ssd1680.c)** - Test program

### Key Functions

```c
// Initialization
bool ssd1680_init(ssd1680_t *dev, const ssd1680_config_t *config);

// Drawing
void ssd1680_draw_pixel(ssd1680_t *dev, int x, int y, uint8_t color);
void ssd1680_draw_rectangle(ssd1680_t *dev, int x0, int y0, int x1, int y1, bool filled);

// Display operations
void ssd1680_clear_screen(ssd1680_t *dev);
void ssd1680_display_frame(ssd1680_t *dev);

// Power management
void ssd1680_sleep(ssd1680_t *dev);
```

---

## Initialization Sequence

The complete initialization sequence is:

1. **Hardware reset** - Toggle RST pin
2. **Software reset** - Command 0x12
3. **Driver output control** - Set MUX to 249 (250 rows)
4. **Data entry mode** - Set to 0x03
5. **RAM X range** - Set to 0-15 (16 bytes)
6. **RAM Y range** - Set to 0-249 (250 rows)
7. **Border waveform** - Set to 0x05
8. **Display update control** - Configure update mode
9. **Temperature sensor** - Enable internal sensor

---

## Testing Results

All tests pass successfully:

✅ **Clear screen** - Complete white, no artifacts
✅ **Full black** - Complete black, no missing areas
✅ **Shapes** - Perfect rectangles, no distortion
✅ **Checkerboard** - Clean patterns, properly aligned
✅ **Corner labels** - Numbers display correctly
✅ **Borders** - Clean lines, no stepping

---

## Common Pitfalls (Avoided)

1. ❌ **Don't use 250×122** - Display is portrait, not landscape
2. ❌ **Don't skip alignment** - Always use 16-byte rows
3. ❌ **Don't forget RED RAM** - Clear both BW and RED RAM
4. ❌ **Don't use wrong data entry mode** - 0x03 is correct for this setup

---

## Next Steps: LVGL Integration

Now that the low-level driver works perfectly, you can integrate with LVGL 9!

The LVGL driver will need to:

1. **Convert colors** - RGB → Monochrome (threshold at brightness < 128)
2. **Handle flush callback** - Copy LVGL buffer to framebuffer
3. **Manage refresh** - Wait for e-paper to complete update
4. **Handle rotation** - Support different orientations

Key LVGL functions to implement:
```c
void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
void lvgl_wait_cb(lv_disp_drv_t *drv);
void lvgl_update_cb(lv_disp_drv_t *drv);
```

---

## Troubleshooting Guide

### If display shows garbage:
1. Check SPI wiring (especially CS and DC pins)
2. Verify BUSY pin connection
3. Ensure SPI speed ≤ 20 MHz

### If shapes are distorted:
1. Verify `SSD1680_WIDTH_BYTES = 16`
2. Check pixel calculation uses `y * SSD1680_WIDTH_BYTES`
3. Confirm data entry mode is 0x03

### If screen doesn't clear:
1. Ensure both BW and RED RAM are cleared
2. Check display update control is 0xF7
3. Verify BUSY pin is working

---

## References

- SSD1680 Datasheet: Solomon Systech
- WeAct Studio 2.13" Display Specifications
- ESP-IDF SPI Master Driver Documentation
- LVGL v9 Display Driver Interface

---

## Summary

**Final working configuration:**
- Portrait orientation: **122 × 250 pixels**
- Aligned rows: **16 bytes per row**
- Data entry mode: **0x03**
- Total buffer: **4000 bytes**

This configuration produces **perfect, undistorted graphics** with no artifacts! 🎉
