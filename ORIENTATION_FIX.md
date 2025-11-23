# Fixing the "Half Black" Display Issue

## The Problem

When you try to fill the entire screen black, only **half appears black**. This is a classic symptom of incorrect display orientation or memory mapping.

## Root Cause

The WeAct 2.13" e-paper display can be mounted in two orientations:
- **Landscape**: 250 pixels wide × 122 pixels tall
- **Portrait**: 122 pixels wide × 250 pixels tall

Our driver assumes **landscape** (250×122), but your display might be physically oriented in **portrait** (122×250), or the controller's RAM mapping doesn't match our assumptions.

---

## Diagnostic Steps

### Step 1: Run Diagnostic Test

I've created a diagnostic test that will help identify the exact issue.

```bash
cd /Users/hvo/Esp32/epaper
./build.sh
./flash.sh
```

### Step 2: Observe the Results

The diagnostic will run 6 tests:

1. **Four corner squares** - Identifies which corner is (0,0)
2. **Horizontal lines** - Tests X-axis addressing
3. **Vertical lines** - Tests Y-axis addressing
4. **Left half black** - Tests X-axis fill
5. **Top half black** - Tests Y-axis fill
6. **Diagonal line** - Tests coordinate mapping

### Step 3: Report What You See

For each test, note:
- Does it appear as expected?
- If not, how is it different?
- Is content rotated 90 degrees?
- Is content mirrored?

---

## Common Issues and Fixes

### Issue 1: Half Black (X-axis problem)

**Symptom:** When filling entire screen black, only left or right half is black

**Cause:** X address range is incorrect

**Fix:** Adjust `SET_RAM_X_ADDRESS_START_END` in initialization

### Issue 2: Half Black (Y-axis problem)

**Symptom:** When filling entire screen black, only top or bottom half is black

**Cause:** Y address range is incorrect

**Fix:** Adjust `SET_RAM_Y_ADDRESS_START_END` in initialization

### Issue 3: 90-Degree Rotation

**Symptom:** Content appears rotated 90 degrees

**Cause:** Display is in portrait mode, we're treating as landscape

**Fix:** Use the swapped driver version

### Issue 4: Stretched or Squashed

**Symptom:** Squares appear as rectangles

**Cause:** Aspect ratio mismatch

**Fix:** Verify physical display dimensions

---

## Using the Swapped Driver

If diagnostics show your display needs swapped orientation:

### Option A: Use Swapped Driver File

```bash
# Edit CMakeLists.txt to use:
idf_component_register(SRCS
    "test_ssd1680.c"
    "ssd1680_lowlevel_swapped.c"  # ← Swapped version
    INCLUDE_DIRS ".")
```

### Option B: Fix Current Driver

Modify [ssd1680_lowlevel.c](main/ssd1680_lowlevel.c) initialization:

```c
// Change Driver Output Control from:
ssd1680_send_data_byte(dev, 0x79);  // 122-1
// To:
ssd1680_send_data_byte(dev, 0xF9);  // 250-1

// Change X address range from:
ssd1680_send_data_byte(dev, 0x1F);  // 31 (for 250 pixels)
// To:
ssd1680_send_data_byte(dev, 0x0F);  // 15 (for 122 pixels)

// Change Y address range from:
ssd1680_send_data_byte(dev, 0x79);  // 121 (for 122 pixels)
// To:
ssd1680_send_data_byte(dev, 0xF9);  // 249 (for 250 pixels)
```

---

## Understanding the Coordinates

### Current (Landscape 250×122)

```
        250 pixels wide
    ┌─────────────────────┐
    │                     │
    │                     │  122 pixels tall
    │        (0,0)        │
    └─────────────────────┘
```

Memory layout:
```
Row 0: Byte 0-31    (250 pixels ÷ 8 = 31.25)
Row 1: Byte 32-63
...
Row 121: Byte 3782-3812
```

### Swapped (Portrait 122×250)

```
    122 pixels wide
    ┌─────────┐
    │         │
    │         │
    │ (0,0)   │  250 pixels tall
    │         │
    │         │
    │         │
    └─────────┘
```

Memory layout:
```
Row 0: Byte 0-15    (122 pixels ÷ 8 = 15.25)
Row 1: Byte 16-31
...
Row 249: Byte 3798-3812
```

---

## Testing Procedure

### 1. Run Diagnostic

```bash
./build.sh && ./flash.sh
```

### 2. Based on Results

If you see:
- ✅ **Four squares in correct corners** → Orientation is correct
- ✅ **Horizontal lines are horizontal** → X-axis is correct
- ✅ **Vertical lines are vertical** → Y-axis is correct
- ✅ **Left half black appears on left** → X addressing is correct
- ✅ **Top half black appears on top** → Y addressing is correct
- ✅ **Diagonal is 45 degrees** → Aspect ratio is correct

If **any** test fails, note which ones and we'll adjust the configuration.

### 3. Common Patterns

| What You See | Problem | Solution |
|--------------|---------|----------|
| Half black (wrong half) | Address range | Adjust start/end addresses |
| Rotated 90° | Orientation | Use swapped driver |
| Mirrored | Scan direction | Change DATA_ENTRY_MODE |
| Stretched | Wrong dimensions | Swap WIDTH/HEIGHT |

---

## Quick Fixes to Try

### Fix 1: Change Data Entry Mode

In [ssd1680_lowlevel.c](main/ssd1680_lowlevel.c):

```c
// Try different data entry modes:
ssd1680_send_data_byte(dev, 0x03);  // Current: X+, Y+ (default)
// Or try:
ssd1680_send_data_byte(dev, 0x00);  // X-, Y-
ssd1680_send_data_byte(dev, 0x01);  // X+, Y-
ssd1680_send_data_byte(dev, 0x02);  // X-, Y+
```

### Fix 2: Change Gate Scan Direction

```c
// In Driver Output Control, third byte:
ssd1680_send_data_byte(dev, 0x00);  // Current: normal scan
// Or try:
ssd1680_send_data_byte(dev, 0x01);  // Inverted scan
```

### Fix 3: Double-Check Physical Display

Measure your physical display:
- Is it wider or taller?
- Which dimension is 2.13 inches (diagonal)?
- Are the connectors on the short or long side?

---

## Next Steps

1. **Run the diagnostic** (`./build.sh && ./flash.sh`)
2. **Observe all 6 tests** carefully
3. **Report back** what you saw
4. I'll help you identify the exact fix needed

The diagnostic will give us precise information about how the display is behaving so we can apply the correct fix!
