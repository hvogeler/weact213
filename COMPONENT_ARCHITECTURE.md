# ESP-IDF Component Architecture

## Overview

The project is organized into **two ESP-IDF components** for clean separation of concerns:

```
project/
├── components/
│   ├── weact_epaper_2in13/       # Low-level hardware driver (Component 1)
│   └── lvgl_weact_epaper/        # LVGL integration layer (Component 2)
├── main/
│   └── main.c                    # Your application
└── managed_components/
    └── lvgl__lvgl/               # LVGL library (from component manager)
```

---

## Component 1: `weact_epaper_2in13`

**Purpose**: Low-level hardware driver for WeAct Studio 2.13" E-Paper Display

### Features
- SSD1680 controller support
- SPI communication (4-wire + control pins)
- Portrait orientation (122×250 pixels)
- 16-byte aligned framebuffer
- Direct pixel and rectangle drawing
- Display refresh and power management

### API

```c
#include "weact_epaper_2in13.h"

// Configuration
weact_epaper_config_t config = {
    .pin_sck = 6,
    .pin_mosi = 7,
    .pin_cs = 10,
    .pin_dc = 9,
    .pin_rst = 4,
    .pin_busy = 18,
    .spi_clock_speed_hz = 4000000,
};

// Initialize
weact_epaper_t display;
weact_epaper_init(&display, &config);

// Draw
weact_epaper_clear_screen(&display);
weact_epaper_draw_pixel(&display, x, y, color);
weact_epaper_draw_rectangle(&display, x0, y0, x1, y1, filled);
weact_epaper_display_frame(&display);

// Power management
weact_epaper_sleep(&display);
```

### Dependencies
- ESP-IDF `driver` component (SPI, GPIO)

### Why Display-Specific?
This is NOT a generic SSD1680 driver. It contains:
- WeAct 2.13" specific initialization sequence
- Hardcoded 122×250 resolution
- Specific LUT timing
- Byte alignment tuned for this display

---

## Component 2: `lvgl_weact_epaper`

**Purpose**: LVGL 9 integration layer for the WeAct display

### Features
- Simple initialization API
- Automatic LVGL display registration
- Color to monochrome conversion (RGB → B/W)
- Flush callback implementation
- Wait callback for e-paper refresh synchronization

### API (Simple!)

```c
#include "lvgl.h"
#include "lvgl_weact_epaper.h"

// Initialize LVGL and display
lv_init();

lvgl_weact_epaper_config_t config = lvgl_weact_epaper_get_default_config();
lv_disp_t *disp = lvgl_weact_epaper_create(&config);

// Now use LVGL normally!
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "Hello E-Paper!");

// Main loop
while (1) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

### Dependencies
- `weact_epaper_2in13` component (low-level driver)
- `lvgl__lvgl` managed component (LVGL library)

### Responsibilities
- Convert LVGL's RGB framebuffer to monochrome
- Copy data to aligned e-paper framebuffer
- Handle display refresh synchronization
- Provide clean API for applications

---

## Dependency Graph

```
main.c
 │
 ├──► lvgl__lvgl (managed component)
 │     └──► LVGL UI widgets, rendering engine
 │
 └──► lvgl_weact_epaper
       │
       ├──► weact_epaper_2in13
       │     └──► driver (ESP-IDF SPI/GPIO)
       │
       └──► lvgl__lvgl
```

---

## Benefits of This Architecture

### ✅ Separation of Concerns
- Hardware control isolated in `weact_epaper_2in13`
- LVGL integration isolated in `lvgl_weact_epaper`
- Application code stays clean

### ✅ Testability
- Test low-level driver independently (without LVGL)
- Test LVGL integration separately
- Easy to create unit tests for each component

### ✅ Reusability
- Use `weact_epaper_2in13` in non-LVGL projects
- Easy to swap LVGL for another GUI library
- Component can be shared across projects

### ✅ Maintainability
- Clear boundaries between layers
- Easy to locate and fix issues
- Simple to understand codebase structure

### ✅ ESP-IDF Best Practices
- Follows ESP-IDF component guidelines
- Clean CMakeLists.txt files
- Proper include directory structure
- No complex Kconfig for display-specific driver

---

## Configuration

### Via Code (Display-Specific Settings)
```c
lvgl_weact_epaper_config_t config = {
    .pin_sck = 6,
    .pin_mosi = 7,
    .pin_cs = 10,
    .pin_dc = 9,
    .pin_rst = 4,
    .pin_busy = 18,
    .spi_clock_speed_hz = 4000000,
};
```

### Via `idf.py menuconfig` (LVGL Settings)
```
Component config → LVGL configuration
```

No Kconfig needed for display driver since it's hardware-specific!

---

## Next Steps

1. ✅ Component 1: `weact_epaper_2in13` created
2. 🔨 Component 2: `lvgl_weact_epaper` implementation (in progress)
3. ⏭️ Update `main.c` to use components
4. ⏭️ Test and verify

---

## File Structure

```
components/
├── weact_epaper_2in13/
│   ├── CMakeLists.txt                 # Component build config
│   ├── README.md                      # Component documentation
│   ├── include/
│   │   └── weact_epaper_2in13.h      # Public API
│   └── weact_epaper_2in13.c          # Implementation
│
└── lvgl_weact_epaper/
    ├── CMakeLists.txt                 # Component build config
    ├── README.md                      # Component documentation  
    ├── include/
    │   └── lvgl_weact_epaper.h       # Public API
    └── lvgl_weact_epaper.c           # LVGL callbacks
```

---

## Summary

**Two-component approach provides:**
- 🎯 Clear separation: hardware vs. GUI integration
- 🧪 Independent testing of each layer
- 🔄 Reusability across projects
- 📦 ESP-IDF best practices
- 🎨 Clean application code

**Display-specific approach provides:**
- ⚡ No complex configuration
- 🎯 Optimized for one display
- 📝 Simpler code
- 🐛 Easier debugging
