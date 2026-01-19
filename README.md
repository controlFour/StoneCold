# Stonecold

A precision temperature controller built on the M5Stack Dial, featuring TEC (Peltier) cooling with PID control, a PT100 RTD sensor, and an LVGL-based circular touchscreen UI.

## Features

- **Precision Temperature Control**: PT100 RTD sensor via MAX31865 for accurate readings
- **PID-Controlled Cooling**: IBT-2 H-bridge drives TEC Peltier cooler with auto-tune support
- **Smart Fan Control**: Automatic fan speed adjustment based on temperature setpoint
- **Circular LVGL UI**: 240x240 round display with rotary encoder navigation
- **Snow Effect**: Ambient falling snow animation when cooling and idle
- **Persistent Settings**: Temperature unit, setpoints, and PID values saved to EEPROM
- **WiFi/OTA Updates**: Optional wireless firmware updates and telnet monitoring

## Hardware

| Component | Description |
|-----------|-------------|
| M5Stack Dial | ESP32-S3 with 240x240 round LCD, rotary encoder |
| MAX31865 | RTD-to-digital converter for PT100 sensor |
| PT100 RTD | 2-wire platinum resistance temperature sensor |
| EXTIO2 | STM32F030-based I2C GPIO expander (I2C address 0x45) |
| IBT-2 | Dual H-bridge motor driver for TEC control |
| TEC Module | Peltier thermoelectric cooler |
| PC Fans (x2) | 4-pin PWM fans for heat dissipation |

### Wiring

See `docs/Stonecold_wiring.png` for the complete wiring diagram.

## Documentation

| Document | Description |
|----------|-------------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | System architecture, class design, pin configuration |
| [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) | Development notes, gotchas, and critical patterns |
| [docs/SNOW_EFFECT.md](docs/SNOW_EFFECT.md) | Snow animation module documentation |
| [docs/BOM.xlsx](docs/BOM.xlsx) | Bill of materials |
| [docs/Stonecold Enclosure.f3z](docs/Stonecold%20Enclosure.f3z) | Fusion 360 enclosure design |

## Flashing Firmware

Pre-built firmware binaries are available in the `/bin` directory. See [bin/README.md](bin/README.md) for flashing instructions using the browser-based ESP Web Tool.

## Building from Source

### Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)

### Build and Upload

```bash
# Build
pio run

# Upload via USB
pio run -t upload

# Monitor serial output
pio device monitor
```

### Configuration

Edit `src/main.cpp` to configure:

```cpp
// Set to 0 to disable WiFi, OTA, and telnet
#define ENABLE_WIFI 1

// WiFi credentials (when ENABLE_WIFI is 1)
const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASSWORD = "your-password";
```

## Usage

### Main Screen
- **Rotary encoder**: Select between setpoint and settings icon
- **Button press**: Enter edit mode (setpoint) or open settings menu

### Setpoint Edit Mode
- **Rotary encoder**: Adjust temperature setpoint
- **Button press**: Save and exit edit mode

### Settings Menu
Navigate to configure:
- Temperature unit (Celsius/Fahrenheit)
- PID parameters (Kp, Ki, Kd)
- Auto-tune PID
- Fan settings (max speed, smart control)
- Current draw monitoring
- Power output display

### Smart Fan Control
When enabled, fans automatically:
- Run at "Smart Setpoint" speed when temperature is at setpoint
- Ramp to "Max Fan" speed when temperature exceeds setpoint by 5°F
- Ramp to full speed when setpoint is changed, until new setpoint is reached

## Project Structure

```
Stonecold/
├── src/                  # Source files
│   ├── main.cpp          # Application entry point
│   ├── PCA9554.cpp       # EXTIO2 I2C GPIO expander
│   ├── TemperatureSensor.cpp
│   ├── TECController.cpp
│   ├── FanController.cpp
│   ├── PIDController.cpp
│   ├── DisplayManager.cpp
│   ├── UIStateMachine.cpp
│   ├── SettingsManager.cpp
│   ├── InputController.cpp
│   └── snow_effect.c     # Snow animation
├── include/              # Header files
├── docs/                 # Documentation and design files
├── bin/                  # Pre-built firmware binaries
├── platformio.ini        # Build configuration
└── lv_conf.h             # LVGL configuration
```

## License

MIT
