# Stonecold Development Notes

## LVGL Screen Transitions (CRITICAL)

When transitioning between screens in LVGL, you MUST follow this pattern:

**ALWAYS: Load new screen BEFORE deleting old screen**

```cpp
void showNewScreen() {
    // 1. Create new screen FIRST
    createNewScreen();

    // 2. Load the new screen
    lv_scr_load(_newScreen);

    // 3. Update visibility flags
    _newVisible = true;
    _oldVisible = false;

    // 4. NOW safe to delete old screen (no longer active)
    if (_oldScreen) {
        lv_obj_del(_oldScreen);
        _oldScreen = nullptr;
        // ... null out all child pointers too
    }
}
```

**WHY:** LVGL may auto-delete or invalidate the old screen when loading a new one. If you delete the old screen first, LVGL has no active screen and may crash or show garbage. Also, old screen pointers may become dangling references if LVGL garbage collects them.

**WRONG (causes crashes):**
```cpp
// DON'T DO THIS
if (_oldScreen) {
    lv_obj_del(_oldScreen);  // Deleting active screen!
}
createNewScreen();
lv_scr_load(_newScreen);
```

## M5Stack Dial Rotary Encoder Fix

The M5Dial library has a bug with the ESP32-S3 GPIO 40/41 encoder pins. The built-in encoder reading doesn't work reliably.

**Solution:** Use the ESP32 PCNT (Pulse Counter) hardware directly:

```cpp
#include "driver/pcnt.h"
#include "driver/gpio.h"

static void initPCNTEncoder() {
    pcnt_config_t enc_config = {
        .pulse_gpio_num = GPIO_NUM_40,  // Rotary Encoder Chan A
        .ctrl_gpio_num = GPIO_NUM_41,   // Rotary Encoder Chan B
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_REVERSE,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .counter_h_lim = INT16_MAX,
        .counter_l_lim = INT16_MIN,
        .unit = PCNT_UNIT_0,
        .channel = PCNT_CHANNEL_0,
    };
    pcnt_unit_config(&enc_config);

    // Second channel for quadrature decoding
    enc_config.pulse_gpio_num = GPIO_NUM_41;
    enc_config.ctrl_gpio_num = GPIO_NUM_40;
    enc_config.channel = PCNT_CHANNEL_1;
    enc_config.pos_mode = PCNT_COUNT_DEC;
    enc_config.neg_mode = PCNT_COUNT_INC;
    pcnt_unit_config(&enc_config);

    // Filter for debouncing
    pcnt_set_filter_value(PCNT_UNIT_0, 250);
    pcnt_filter_enable(PCNT_UNIT_0);

    // Enable pull-ups
    gpio_pullup_en(GPIO_NUM_40);
    gpio_pullup_en(GPIO_NUM_41);

    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
}

static int16_t getPCNTEncoder() {
    int16_t count;
    pcnt_get_counter_value(PCNT_UNIT_0, &count);
    return count;
}
```

Then track detents (the M5Dial encoder has 4 pulses per detent):
```cpp
static constexpr int PULSES_PER_DETENT = 4;

int getEncoderDelta() {
    int32_t currentPos = getPCNTEncoder();
    int32_t diff = currentPos - _lastActionPos;

    if (diff >= PULSES_PER_DETENT) {
        _lastActionPos = currentPos;
        return 1;
    } else if (diff <= -PULSES_PER_DETENT) {
        _lastActionPos = currentPos;
        return -1;
    }
    return 0;
}
```

## PID Auto-Tune for TEC Systems

Classic Ziegler-Nichols tuning is too aggressive for TEC (thermoelectric cooler) systems. Use conservative "No Overshoot" tuning:

```cpp
// After relay auto-tune determines Tu (ultimate period) and Ku (ultimate gain):
float newKp = 0.2f * Ku;           // Much less aggressive than 0.6 * Ku
float newKi = 0.4f * Ku / Tu;      // Less aggressive than 1.2 * Ku / Tu
float newKd = newKp * 0.25f;       // D = 25% of P (thermal-friendly ratio)
```

**Why:** TEC systems have slow thermal response and noisy temperature sensors. High derivative gain causes oscillations. High proportional gain causes overshoot which can damage temperature-sensitive components.

## Snow Effect Memory Management (CRITICAL)

The snow effect creates LVGL canvas objects as children of the main screen. **These objects are deleted when the main screen is deleted.** This causes crashes if not handled properly.

**CRITICAL: Deinit snow BEFORE any screen transition that deletes the main screen**

The problem occurs because:
1. Button press triggers screen transition (e.g., main → setpoint screen)
2. `showSetpointScreen()` deletes the main screen
3. Snow canvas objects (children of main screen) are now invalid/deleted
4. Later code tries to access snow objects → CRASH

**Solution in main.cpp:**
```cpp
// BEFORE processing UI input, check if button was pressed
if (snowInitialized && input.wasButtonPressed()) {
    snow_effect_deinit();
    snowInitialized = false;
    input.requeueButtonPress();  // Re-queue so UI still gets it
}

// NOW safe to process UI (screen transitions won't crash)
input.update();
ui.update();
```

**Key points:**
1. Check for button press BEFORE `ui.update()` processes it
2. Deinit snow BEFORE the screen transition can happen
3. Re-queue the button press so the UI still receives it
4. Only reinitialize snow when back on main screen AND conditions are met (inactive, cooling active)

**WRONG (causes crashes):**
```cpp
// DON'T DO THIS - snow deinit happens AFTER screen is deleted
ui.update();  // This deletes main screen!
// ... later ...
if (!shouldShowSnow && snowInitialized) {
    snow_effect_deinit();  // TOO LATE - objects already deleted!
}
```
