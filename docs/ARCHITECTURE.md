# Stonecold - Architecture Documentation

## Project Overview

Stonecold is an M5Stack Dial-based temperature controller with a PT100 RTD sensor. It features a modern LVGL-based UI with snow effect animations, rotary encoder navigation, and persistent settings.

### Hardware Platform
- **MCU**: ESP32-S3 (M5Stack Dial)
- **Display**: 240x240 round LCD with LVGL graphics
- **Input**: Rotary encoder (4 pulses per detent) + center button
- **Audio**: Built-in speaker for UI feedback
- **Temperature Sensor**: MAX31865 RTD-to-Digital converter with PT100 (2-wire)
- **I/O Expander**: EXTIO2 (STM32F030) on I2C for software SPI and GPIO expansion
- **TEC Driver**: IBT-2 H-bridge for Peltier cooler control

### Pin Configuration
| Function | Pin | Notes |
|----------|-----|-------|
| Port A SDA | GPIO13 | External I2C for PCA9554 |
| Port A SCL | GPIO15 | External I2C for PCA9554 |
| Port B (RPWM) | GPIO1 | PWM output for IBT-2 |
| EXTIO2 Address | 0x45 | I2C address (0x54 in bootloader mode) |
| MAX31865 CLK | PCA9554 Pin 0 | Software SPI clock |
| MAX31865 SDO (MISO) | PCA9554 Pin 1 | Data from MAX31865 |
| MAX31865 SDI (MOSI) | PCA9554 Pin 2 | Data to MAX31865 |
| MAX31865 CS | PCA9554 Pin 3 | Chip select (active low) |
| IBT-2 REN | PCA9554 Pin 4 | TEC enable (active high) |

---

## File Structure

```
Stonecold/
├── include/                    # Header files
│   ├── PCA9554.h               # I2C I/O expander (shared)
│   ├── SettingsManager.h       # Temperature unit, EEPROM persistence
│   ├── TemperatureSensor.h     # RTD sensor hardware abstraction
│   ├── TECController.h         # TEC/Peltier control via IBT-2
│   ├── InputController.h       # Encoder and button input
│   ├── DisplayManager.h        # LVGL display and screens
│   └── UIStateMachine.h        # UI mode and navigation logic
│
├── src/                        # Implementation files
│   ├── main.cpp                # Application entry point (~90 lines)
│   ├── PCA9554.cpp
│   ├── SettingsManager.cpp
│   ├── TemperatureSensor.cpp
│   ├── TECController.cpp
│   ├── InputController.cpp
│   ├── DisplayManager.cpp
│   ├── UIStateMachine.cpp
│   │
│   ├── snow_effect.c           # Snow animation (C code)
│   ├── snow_effect.h
│   │
│   ├── settings_img.c          # Settings gear icon (LVGL image)
│   ├── settings_img.h
│   ├── snowflake_img.c         # Snowflake icon (LVGL image)
│   ├── snowflake_img.h
│   │
│   └── lv_font_montserrat_*.c  # Custom font sizes (60, 72, 84, 96)
│
├── platformio.ini              # Build configuration
├── lv_conf.h                   # LVGL configuration
└── ARCHITECTURE.md             # This file
```

---

## Class Architecture

All classes use the **Meyer's Singleton** pattern for global access:

```cpp
class Example {
public:
    static Example& getInstance() {
        static Example instance;
        return instance;
    }
    void begin();  // Initialize (call in setup())
    void update(); // Update (call in loop() if needed)
private:
    Example() = default;
    Example(const Example&) = delete;
    Example& operator=(const Example&) = delete;
};
```

### Class Dependency Graph

```
main.cpp
    │
    ├── PCA9554              (no dependencies - I2C I/O expander)
    │
    ├── SettingsManager      (no dependencies)
    │
    ├── TemperatureSensor ──► PCA9554 (for software SPI)
    │
    ├── TECController ──────► PCA9554 (for REN pin)
    │
    ├── InputController      (no dependencies)
    │
    ├── DisplayManager ────► SettingsManager (for unit conversion)
    │
    └── UIStateMachine ────► SettingsManager
                       ────► TemperatureSensor
                       ────► InputController
                       ────► DisplayManager
```

---

## Class Details

### PCA9554 (EXTIO2)
**File**: `include/PCA9554.h`, `src/PCA9554.cpp`

Shared I2C I/O expander (EXTIO2 unit with STM32F030) for multiple peripherals. Manages output state to prevent conflicts.

**Important**: The EXTIO2 requires custom firmware (version 5) for fan RPM counting and PWM control. The stock M5Stack firmware does not support these modes. Update via Settings → Firmware → Update GPIO FW.

```cpp
// Key methods
void begin();                              // Initialize (all pins as inputs)
void setPinMode(uint8_t pin, bool output); // Configure pin direction
void digitalWrite(uint8_t pin, bool level);// Set output pin
bool digitalRead(uint8_t pin);             // Read input pin
void setFanRPMPinMode(uint8_t pin);        // Custom firmware: FAN_RPM mode
void setPWMPinMode(uint8_t pin);           // Custom firmware: PWM output mode
void setPWMFrequency(uint8_t freqMode);    // Set PWM frequency (1=1kHz)
void setPWMDutyCycle(uint8_t pin, uint8_t percent); // Set PWM duty cycle
uint16_t readFanRPM(uint8_t pin);          // Read fan RPM from tach pin
```

**Pin Allocation**:
| Pin | Function | Mode |
|-----|----------|------|
| 0 | MAX31865 CLK | Digital Output |
| 1 | MAX31865 SDO (MISO) | Digital Input |
| 2 | MAX31865 SDI (MOSI) | Digital Output |
| 3 | MAX31865 CS | Digital Output |
| 4 | IBT-2 REN | Digital Output |
| 5 | Fan 1 Tach | FAN_RPM (custom firmware) |
| 6 | Fan 2 Tach | FAN_RPM (custom firmware) |
| 7 | Fan PWM | PWM Output (custom firmware) |

**Custom Firmware Modes** (register 0x00-0x07):
| Mode | Value | Description |
|------|-------|-------------|
| DIGITAL_INPUT | 0 | Standard digital input |
| DIGITAL_OUTPUT | 1 | Standard digital output |
| ADC_INPUT | 2 | Analog input (10-bit) |
| SERVO_CTL | 3 | Servo PWM output |
| RGB_LED | 4 | NeoPixel control |
| PWM_OUTPUT | 5 | General PWM output |
| FAN_RPM | 6 | Fan tachometer with hardware counting |

---

### SettingsManager
**File**: `include/SettingsManager.h`, `src/SettingsManager.cpp`

Manages persistent settings and temperature unit conversion.

```cpp
// Key methods
void begin();                              // Initialize EEPROM, load settings
void save();                               // Save to EEPROM
TempUnit getTempUnit() const;              // CELSIUS or FAHRENHEIT
void toggleTempUnit();                     // Toggle and save
float toDisplayUnit(float celsius) const;  // Convert for display
const char* getUnitString() const;         // "C" or "F"
```

**EEPROM Layout**:
| Address | Size | Content |
|---------|------|---------|
| 0 | 1 byte | Temperature unit (0=Celsius, 1=Fahrenheit) |

---

### TemperatureSensor
**File**: `include/TemperatureSensor.h`, `src/TemperatureSensor.cpp`

Hardware abstraction for PCA9554 I/O expander and MAX31865 RTD sensor.

```cpp
// Key methods
void begin();              // Initialize PCA9554 and MAX31865
float readTemperature();   // Returns temperature in Celsius
```

**Sensor Constants**:
| Constant | Value | Description |
|----------|-------|-------------|
| RTD_RREF | 430.0 | Reference resistor (Adafruit board) |
| RTD_NOMINAL | 100.0 | PT100 resistance at 0°C |
| RTD_A | 3.9083e-3 | Callendar-Van Dusen coefficient |
| RTD_B | -5.775e-7 | Callendar-Van Dusen coefficient |

**I2C Note**: M5Dial's external I2C must be released before using Wire:
```cpp
M5.Ex_I2C.release();
Wire.begin(13, 15);  // Port A: SDA=13, SCL=15
```

---

### TECController
**File**: `include/TECController.h`, `src/TECController.cpp`

Controls TEC (Peltier cooler) via IBT-2 H-bridge motor driver.

```cpp
// Key methods
void begin();                  // Initialize PWM and enable pin
void setEnabled(bool enabled); // Enable/disable TEC
void setPower(float power);    // Set cooling power (0.0 to 1.0)
void stop();                   // Disable and set power to 0
bool isEnabled() const;        // Check if enabled
float getPower() const;        // Get current power setting
```

**Hardware Configuration**:
| Signal | Pin | Description |
|--------|-----|-------------|
| RPWM | GPIO1 (Port B) | PWM for cooling direction |
| REN | PCA9554 Pin 4 | Right channel enable |

**PWM Settings**:
- Frequency: 25kHz (inaudible)
- Resolution: 8-bit (0-255)
- LEDC Channel: 0

**IBT-2 Notes**:
- The IBT-2 is a dual H-bridge capable of 43A peak current
- For TEC control, typically only one direction (cooling) is used
- REN must be HIGH for the RPWM signal to drive the output
- Future: LPWM/LEN can be added for heating mode, IS pins for current sensing

---

### InputController
**File**: `include/InputController.h`, `src/InputController.cpp`

Handles rotary encoder input with detent detection and audio feedback.

```cpp
// Key methods
void begin();                // Initialize encoder
void update();               // Call after M5Dial.update()
int getEncoderDelta();       // Detents rotated since last call
bool wasButtonPressed();     // Button press since last call
void playNavigationBeep();   // 6kHz, 10ms
void playEnterBeep();        // 8kHz, 20ms
void playExitBeep();         // 4kHz, 20ms
void playToggleBeep();       // 7kHz, 20ms
```

**Encoder**: M5Dial encoder generates 4 pulses per detent. The class accumulates pulses and returns whole detent counts.

---

### DisplayManager
**File**: `include/DisplayManager.h`, `src/DisplayManager.cpp`

Manages LVGL initialization, screens, and UI elements.

```cpp
// Key methods
void begin();                           // Initialize LVGL and create main screen
void update();                          // Call lv_timer_handler()
void showMainScreen();                  // Switch to main screen
void updateMainScreen(temp, setpoint, selection, editing);
void showSettingsScreen();              // Switch to settings screen
void updateSettingsScreen(selectedItem);
void closeSettingsScreen();             // Return to main screen
lv_obj_t* getActiveScreen();            // For snow effect attachment
```

**Screens**:
1. **Main Screen**: Temperature display, setpoint, settings icon
2. **Settings Screen**: Temperature unit toggle, PID (placeholder), Back

---

### UIStateMachine
**File**: `include/UIStateMachine.h`, `src/UIStateMachine.cpp`

Controls UI modes, navigation, and application state.

```cpp
// Key methods
void begin();                    // Initialize state
void update();                   // Process input, update display
float getSetpoint() const;       // Current setpoint (Celsius)
float getCurrentTemp() const;    // Last temperature reading
bool isInactive() const;         // True after 10s of no input
void resetInactivityTimer();     // Reset on user interaction
```

**UI Modes**:
| Mode | Description | Encoder Action | Button Action |
|------|-------------|----------------|---------------|
| MODE_NAVIGATE | Select setpoint or settings | Toggle selection | Enter edit/settings |
| MODE_EDIT | Adjust setpoint | Change setpoint ±1°C | Exit to navigate |
| MODE_SETTINGS | Settings menu | Navigate items | Activate item |

**Setpoint Range**: 10°C to 35°C

---

## Snow Effect (C Module)

**Files**: `src/snow_effect.c`, `src/snow_effect.h`

Ambient snow animation that plays during inactivity.

```c
// API
void snow_effect_init(lv_obj_t* parent);  // Initialize on screen
void snow_effect_deinit();                 // Clean up resources
void snow_effect_manual_update();          // Call at ~33 FPS
void snow_effect_pause_and_hide();         // Pause on user activity
void snow_effect_resume_and_show();        // Resume after inactivity
bool snow_effect_is_paused();              // Check pause state
```

**Configuration** (in snow_effect.c):
- 20 snowflakes
- Wind simulation with smooth transitions
- Depth-based rendering (size/opacity varies)
- Sine-wave horizontal drift

**Inactivity Delay**: 10 seconds (defined in UIStateMachine)

---

## UI Styling

### Color Palette
| Element | Color | Hex |
|---------|-------|-----|
| Background | Dark gray | `0x1a1a1a` |
| Temperature text | White | `0xffffff` |
| Setpoint (normal) | Cyan | `0x00aaff` |
| Setpoint (selected) | Yellow | `0xffff00` |
| Setpoint (editing) | Green | `0x00ff00` |
| Settings icon (normal) | Gray | `0x888888` |
| Settings icon (selected) | Yellow | `0xffff00` |
| Settings item (normal) | Gray | `0x888888` |
| Settings item (selected) | Yellow | `0xffff00` |

### Fonts
| Element | Font | Size |
|---------|------|------|
| Temperature | Montserrat | 96px |
| Setpoint | Montserrat | 20px |
| Settings title | Montserrat | 20px |
| Temperature unit (settings) | Montserrat | 48px |
| Settings items | Montserrat | 20px |

### Layout (240x240 screen)
```
┌─────────────────────────┐
│      Setpoint (top)     │  y=20
│                         │
│                         │
│   Temperature (center)  │  y=120
│                         │
│                         │
│   Settings Icon (bottom)│  y=220
└─────────────────────────┘
```

---

## Build Configuration

### platformio.ini Key Settings
```ini
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

build_flags =
    -DARDUINO_M5STACK_DIAL
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DLV_CONF_PATH="${PROJECT_DIR}/lv_conf.h"

lib_deps =
    m5stack/M5Unified@^0.2.2
    m5stack/M5Dial@^1.0.2
    lvgl/lvgl@^8.3.11

board_build.partitions = huge_app.csv
```

### Memory Usage (typical)
- RAM: ~29% (95KB / 328KB)
- Flash: ~28% (897KB / 3.1MB)

---

## Extending the Project

### Adding a New Setting
1. Add EEPROM address constant in `SettingsManager.h`
2. Add getter/setter methods in `SettingsManager`
3. Update `save()` and `load()` methods
4. Add UI element in `DisplayManager::createSettingsScreen()`
5. Handle in `UIStateMachine::handleSettingsButtonPress()`

### Adding a New Screen
1. Add screen pointer in `DisplayManager` private members
2. Create `createNewScreen()` method
3. Add `showNewScreen()` and `updateNewScreen()` methods
4. Add new UIMode in `UIStateMachine`
5. Handle mode transitions in `UIStateMachine::update()`

### Changing Temperature Sensor
1. Modify `TemperatureSensor` class
2. Update `begin()` for new sensor initialization
3. Update `readTemperature()` for new reading method
4. Adjust calibration constants as needed

---

## Troubleshooting

### I2C Issues
- Ensure `M5.Ex_I2C.release()` is called before `Wire.begin()`
- Use `Wire.endTransmission(false)` for repeated start condition
- PCA9554 address is 0x27 for M5Stack EXT.IO unit

### Display Issues
- Call `lv_timer_handler()` frequently in loop
- Use `lv_refr_now(nullptr)` for immediate refresh
- Invalidate objects after changing properties

### Encoder Issues
- M5Dial encoder: 4 pulses per detent
- Always call `M5Dial.update()` before reading encoder
- Accumulate pulses to detect full detents

---

## Version History

| Date | Changes |
|------|---------|
| 2024-12-24 | Initial OOP refactoring from monolithic main.cpp |
| 2024-12-24 | Added MAX31865 RTD support via PCA9554 I/O expander |
