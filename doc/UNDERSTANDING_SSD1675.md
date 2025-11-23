# Understanding the SSD1675 Low-Level Driver

This document explains how the SSD1675 e-paper driver works from the ground up.

## Table of Contents
1. [Hardware Communication Basics](#hardware-communication-basics)
2. [Understanding the Code Flow](#understanding-the-code-flow)
3. [Memory Layout and Bit Manipulation](#memory-layout-and-bit-manipulation)
4. [Display Refresh Process](#display-refresh-process)
5. [Next Steps: LVGL Integration](#next-steps-lvgl-integration)

---

## Hardware Communication Basics

### SPI Protocol Overview

SPI (Serial Peripheral Interface) is a synchronous serial communication protocol. Think of it as a conversation where:
- **Master (ESP32)** controls the conversation timing
- **Slave (SSD1675)** responds when addressed

```
Timing Diagram:
       ____      ____      ____      ____
SCK  _|    |____|    |____|    |____|    |____  (Clock pulses)

MOSI ___/¯¯¯¯\___/¯¯¯¯\___/¯¯¯¯\___/¯¯¯¯\____  (Data bits from ESP32)
     Bit7   Bit6   Bit5   Bit4   ... (MSB first)
```

### Control Signals

1. **CS (Chip Select)**
   - LOW = "Hey SSD1675, listen to me!"
   - HIGH = "Ignore all communication"

2. **DC (Data/Command)**
   - LOW = "I'm sending a command byte"
   - HIGH = "I'm sending data bytes"

3. **BUSY**
   - HIGH = "I'm busy refreshing the screen"
   - LOW = "I'm ready for new commands"

### Example: Sending a Command with Data

Let's trace what happens when we clear the screen:

```c
// Step 1: Send "WRITE_RAM" command
ssd1675_send_command(dev, SSD1675_CMD_WRITE_RAM);
```

What happens internally:
```
1. Set DC = LOW        (command mode)
2. Set CS = LOW        (activate chip)
3. Clock out 0x24      (WRITE_RAM command byte)
4. Set CS = HIGH       (deactivate)
```

```c
// Step 2: Send framebuffer data
ssd1675_send_data(dev, framebuffer, 3813);  // 250×122÷8 = 3,812.5 → 3,813 bytes
```

What happens internally:
```
1. Set DC = HIGH       (data mode)
2. Set CS = LOW        (activate chip)
3. Clock out byte 0    (first 8 pixels)
4. Clock out byte 1    (next 8 pixels)
... (3,813 bytes total)
5. Set CS = HIGH       (deactivate)
```

---

## Understanding the Code Flow

### Initialization Sequence

When you call `ssd1675_init()`, here's what happens step by step:

```c
bool ssd1675_init(ssd1675_t *dev, const ssd1675_config_t *config)
```

#### Phase 1: Hardware Setup (Lines 121-150)
```c
// Configure GPIO pins (DC, RST as outputs; BUSY as input)
gpio_config_t io_conf = { ... };
```
**Why?** GPIO pins need to be configured before use. Think of it as "declaring" what each pin does.

#### Phase 2: SPI Bus Setup (Lines 152-178)
```c
spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
```
**Why DMA?** Direct Memory Access allows SPI transfers without CPU intervention = faster!

#### Phase 3: Memory Allocation (Lines 180-189)
```c
dev->framebuffer = heap_caps_malloc(SSD1675_BUFFER_SIZE, MALLOC_CAP_DMA);
```
**Why DMA-capable memory?** The SPI controller will read directly from this memory using DMA.

**Memory calculation:**
```
Resolution: 250 × 122 pixels
Bits needed: 250 × 122 = 30,500 bits
Bytes needed: 30,500 ÷ 8 = 3,812.5 → 3,813 bytes (round up)
```

#### Phase 4: Display Initialization (Lines 200-253)

This is where we "teach" the SSD1675 controller about our display:

```c
// Tell it: "You have 122 gate lines"
ssd1675_send_command(dev, SSD1675_CMD_DRIVER_OUTPUT_CONTROL);
ssd1675_send_data_byte(dev, 0x79);  // 121 (because 0-indexed)
```

```c
// Tell it: "Data flows left→right, top→bottom"
ssd1675_send_command(dev, SSD1675_CMD_DATA_ENTRY_MODE);
ssd1675_send_data_byte(dev, 0x03);
```

**Data Entry Mode Bits:**
```
0x03 = 0b00000011
           ||||└─ ID[0]: X increment
           |||└── ID[1]: Y increment
           ||└─── AM: 0 = X direction first
           └───── (unused)
```

```c
// Tell it: "X addresses go from 0 to 31"
ssd1675_send_command(dev, SSD1675_CMD_SET_RAM_X_ADDRESS_RANGE);
ssd1675_send_data_byte(dev, 0x00);  // Start: 0
ssd1675_send_data_byte(dev, 0x1F);  // End: 31 (250 pixels ÷ 8 = 31.25)
```

**Why 31?** X address is in BYTES, not pixels:
- 250 pixels wide
- 8 pixels per byte
- 250 ÷ 8 = 31.25 → address 0 to 31

---

## Memory Layout and Bit Manipulation

### The Framebuffer

The framebuffer is a 1D array representing a 2D image:

```
Physical Display:         Framebuffer in Memory:
┌──────────────┐         [byte 0][byte 1][byte 2]...[byte 31] ← Row 0
│ 250 × 122    │         [byte 32][byte 33]...[byte 63]       ← Row 1
│ pixels       │         ...
└──────────────┘         [byte 3782]...[byte 3812]            ← Row 121
```

### Pixel Addressing

To draw a pixel at coordinate (x, y):

```c
void ssd1675_draw_pixel(ssd1675_t *dev, int x, int y, uint8_t color)
{
    // Step 1: Find which byte contains this pixel
    int byte_index = (y * SSD1675_WIDTH + x) / 8;

    // Step 2: Find which bit within that byte
    int bit_index = x % 8;

    // Step 3: Set or clear the bit
    if (color == 0) {
        // WHITE: Set bit to 1
        dev->framebuffer[byte_index] |= (1 << (7 - bit_index));
    } else {
        // BLACK: Clear bit to 0
        dev->framebuffer[byte_index] &= ~(1 << (7 - bit_index));
    }
}
```

#### Example: Drawing pixel at (10, 5)

**Step 1: Calculate byte index**
```
byte_index = (5 × 250 + 10) / 8
           = (1250 + 10) / 8
           = 1260 / 8
           = 157
```

**Step 2: Calculate bit position**
```
bit_index = 10 % 8 = 2
```

**Step 3: Which bit to modify?**
```
Byte 157:  [7][6][5][4][3][2][1][0]  ← Bit positions
                          ^
                          We modify bit 2 (from the right)
                          But we count 7-2=5 from the left!
```

**Why `7 - bit_index`?**
- Pixels go left-to-right on screen
- Bits in a byte go from MSB (bit 7) to LSB (bit 0)
- Pixel at x=0 → bit 7 (leftmost)
- Pixel at x=7 → bit 0 (rightmost)

### Visual Example

```
Pixels on screen:  [P0][P1][P2][P3][P4][P5][P6][P7]
                    ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓
Byte in memory:    [7] [6] [5] [4] [3] [2] [1] [0]
```

If we want to draw a pattern: BLACK WHITE WHITE BLACK BLACK BLACK WHITE BLACK

```
Color values:      1    0    0    1    1    1    0    1
Byte value:      0b10011101 = 0x9D
```

---

## Display Refresh Process

### The Complete Journey: From Framebuffer to E-Paper

When you call `ssd1675_display_frame()`, here's the entire process:

#### Step 1: Set RAM Write Position

```c
// Move cursor to (0, 0)
ssd1675_send_command(dev, SSD1675_CMD_SET_RAM_X_ADDRESS_COUNTER);
ssd1675_send_data_byte(dev, 0x00);

ssd1675_send_command(dev, SSD1675_CMD_SET_RAM_Y_ADDRESS_COUNTER);
ssd1675_send_data_byte(dev, 0x00);
ssd1675_send_data_byte(dev, 0x00);
```

**Think of it like:** Moving a text cursor to the start of a document before typing.

#### Step 2: Upload Framebuffer

```c
ssd1675_send_command(dev, SSD1675_CMD_WRITE_RAM);
ssd1675_send_data(dev, dev->framebuffer, SSD1675_BUFFER_SIZE);
```

This sends all 3,813 bytes to the SSD1675's internal RAM.

**Important:** At this point, the display HASN'T changed yet! The data is just in the controller's memory.

#### Step 3: Trigger Physical Refresh

```c
// Configure the update sequence
ssd1675_send_command(dev, SSD1675_CMD_DISPLAY_UPDATE_CONTROL_2);
ssd1675_send_data_byte(dev, 0xF7);

// Start the refresh!
ssd1675_send_command(dev, SSD1675_CMD_MASTER_ACTIVATION);
```

**What happens now:**
1. The SSD1675 starts applying voltage patterns to the display
2. The BUSY pin goes HIGH
3. Electric charges move black/white particles in the e-ink
4. This takes 1-3 seconds (visible flickering)
5. The BUSY pin goes LOW when complete

#### Step 4: Wait for Completion

```c
ssd1675_wait_until_idle(dev);
```

This polls the BUSY pin until refresh completes.

---

## Understanding the LUT (Look-Up Table)

The LUT is like a "recipe" for moving the ink particles:

```c
static const uint8_t ssd1675_lut_full_update[] = {
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,  // Phase 0
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,  // Phase 1
    ...
};
```

### What Each Phase Does

**Phase 0: White to Black transition**
- Voltage levels: 0x80, 0x60, 0x40 (stepping voltages)
- Duration: controlled by timing bytes

**Phase 1: Black to White transition**
- Different voltage pattern

**Timing bytes (end of LUT):**
```c
0x03, 0x03, 0x00, 0x00, 0x02,  // How long each phase lasts
```

**Analogy:** Like a washing machine cycle:
1. Fill with water (voltage HIGH)
2. Agitate (voltage oscillates)
3. Rinse (voltage LOW)
4. Spin (repeat pattern)

---

## Next Steps: LVGL Integration

Now that you understand the low-level driver, here's what we'll do for LVGL:

### What LVGL Needs

1. **Flush Callback**: Convert LVGL's color buffer → monochrome bitmap → send to display
2. **Wait Callback**: Tell LVGL to pause while e-paper refreshes
3. **Display Driver Registration**: Connect everything together

### The Bridge We'll Build

```
LVGL Rendering          Your Low-Level Driver
─────────────          ───────────────────────

lv_color_t buffer      →  Convert to mono  →  framebuffer
(RGB565 colors)           (threshold)          (1-bit B&W)
                                                    ↓
                                              ssd1675_display_frame()
```

### Key Challenge

LVGL renders in **color** (even if grayscale), but e-paper is **1-bit** (black or white only).

We'll solve this by **thresholding**:
```c
if (brightness < 128) {
    pixel = BLACK;  // Dark enough
} else {
    pixel = WHITE;  // Too light
}
```

---

## Summary: What You've Learned

✅ **SPI Communication**: How ESP32 talks to SSD1675
✅ **Command/Data Protocol**: Using the DC pin
✅ **Memory Layout**: How pixels map to bytes and bits
✅ **Bit Manipulation**: Setting individual pixels
✅ **Refresh Process**: The complete journey from RAM to e-paper
✅ **LUT**: How voltage waveforms control ink particles

### Test Your Understanding

Try these experiments:

1. **Modify the square size** in [test_square.c](../main/test_square.c)
2. **Draw a diagonal line** from (0,0) to (249,121)
3. **Create a gradient effect** using checkerboard with different square sizes
4. **Measure refresh time** by adding timing code

---

## Ready for LVGL?

When you're ready, we'll create the LVGL 9 driver that uses this low-level foundation. You'll learn:

- How LVGL's rendering pipeline works
- Color space conversion (RGB → Monochrome)
- Double buffering for smooth updates
- Handling rotation and mirroring
- Optimizing for e-paper characteristics

Let me know when you want to continue!
