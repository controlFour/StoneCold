# Snow Effect Module

A realistic falling snow effect for LVGL displays on M5Stack Dial.

## Files

- `src/snow_effect.h` - Public API header
- `src/snow_effect.c` - Implementation
- `generate_simple_snowflake.py` - Generates a simple circular snowflake PNG
- `snowflake_simple.png` - Source snowflake image (12x12 circle)

## Features

- **Depth simulation**: Snowflakes have varying size, speed, and opacity to simulate 3D depth
- **Realistic motion**:
  - Vertical falling at different speeds (0.8 - 2.5 pixels/frame)
  - Horizontal drift using sine wave oscillation
  - Slowly changing wind bias for natural variation
  - Rotation support (for future shaped snowflakes)
- **Efficient rendering**:
  - Uses simple LVGL objects styled as circles
  - 20 snowflakes running at ~33 FPS
  - Minimal memory and CPU usage
- **Layering**: Snowflakes render behind UI elements

## Usage

```cpp
#include "snow_effect.h"

// In setup(), after creating your UI:
lv_obj_t *screen = lv_scr_act();
snow_effect_init(screen);

// In loop(), call manual update:
if (millis() - last_snow_update > 30) {
    snow_effect_manual_update();
    last_snow_update = millis();
}

// Clean up
snow_effect_deinit();
```

## Configuration

Edit `snow_effect.c` to adjust:

- `SNOWFLAKE_COUNT` (default: 20) - Number of snowflakes
- `UPDATE_PERIOD_MS` (default: 30) - Target update period (~33 FPS)
- `WIND_CHANGE_SPEED` (default: 0.005) - How quickly wind changes
- Speed ranges (0.8-2.5), size ranges (6-14px diameter), opacity (180-255)

## How It Works

1. **Initialization**: Creates 20 LVGL objects styled as white circles:
   - Each snowflake is sized based on its depth (6-14px diameter)
   - Random position, speed, size, opacity, drift parameters
   - Styled with `LV_RADIUS_CIRCLE` for perfect circles

2. **Update Loop** (called manually from main loop):
   - Updates Y position (falling based on speed)
   - Updates X position (sine drift + wind bias)
   - Updates rotation counter (for future use with shaped snowflakes)
   - Respawns flakes that leave screen boundaries

3. **Depth Effect**:
   - Faster flakes = closer (larger 14px, more opaque)
   - Slower flakes = farther (smaller 6px, more transparent)

## Implementation Notes

- **Manual Update**: Uses `snow_effect_manual_update()` called from main loop for reliable timing
- **Simple Objects**: Each snowflake is a white circle using `lv_obj` with `LV_RADIUS_CIRCLE` styling
- **Simple Respawn**: When leaving screen, flakes reset position/speed without recreating objects
- **Important**: Disable screen scrollbars to prevent gray lines at edges:
  ```cpp
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
  ```

## Customizing Snowflakes

Edit `snow_effect.c` to adjust:
- Snowflake count, size range (6-14px), speed, opacity
- Wind behavior, drift patterns
- For shaped snowflakes: modify `init_snowflake()` to draw custom shapes

## Performance

- 20 snowflakes @ 33 FPS
- Low CPU usage (~30ms per frame)
- Minimal memory (objects created once at init)
- Smooth animation alongside temperature UI
