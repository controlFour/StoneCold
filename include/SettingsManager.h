#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>

enum TempUnit {
    CELSIUS,
    FAHRENHEIT
};

enum PIDMode {
    PID_OFF = 0,
    PID_ON = 1,
    PID_AUTOTUNE = 2
};

class SettingsManager {
public:
    static SettingsManager& getInstance();

    void begin();
    void save();
    void load();

    TempUnit getTempUnit() const;
    void setTempUnit(TempUnit unit);
    void toggleTempUnit();

    // Setpoint (stored in Celsius)
    float getSetpoint() const;
    void setSetpoint(float celsius);

    // Fan speed (0-100%)
    float getFanSpeed() const;
    void setFanSpeed(float percent);

    // Smart control settings
    bool getSmartControlEnabled() const;
    void setSmartControlEnabled(bool enabled);
    float getSmartSetpoint() const;
    void setSmartSetpoint(float percent);

    // PID settings
    PIDMode getPIDMode() const;
    void setPIDMode(PIDMode mode, bool saveNow = true);
    float getPIDKp() const;
    float getPIDKi() const;
    float getPIDKd() const;
    void setPIDTunings(float kp, float ki, float kd, bool saveNow = true);
    float getPIDMinOutput() const;
    float getPIDMaxOutput() const;
    void setPIDOutputLimits(float min, float max, bool saveNow = true);

    // Temperature conversion helpers
    float toDisplayUnit(float celsius) const;
    float celsiusToFahrenheit(float celsius) const;
    float fahrenheitToCelsius(float fahrenheit) const;
    const char* getUnitString() const;
    const char* getUnitSymbol() const;

private:
    SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    TempUnit _tempUnit = CELSIUS;
    float _setpoint = 0.0f;  // Stored in Celsius
    float _fanSpeed = 100.0f;  // Fan speed 0-100%
    bool _smartControlEnabled = false;
    float _smartSetpoint = 50.0f;  // Smart control setpoint 0-100%

    // PID settings
    PIDMode _pidMode = PID_OFF;
    float _pidKp = 0.0f;
    float _pidKi = 0.55f;
    float _pidKd = 2.60f;
    float _pidMinOutput = 0.0f;
    float _pidMaxOutput = 100.0f;

    static constexpr int EEPROM_SIZE = 64;
    static constexpr int EEPROM_ADDR_TEMP_UNIT = 0;
    static constexpr int EEPROM_ADDR_SETPOINT = 1;   // 4 bytes for float
    static constexpr int EEPROM_ADDR_PID_MODE = 5;   // 1 byte
    static constexpr int EEPROM_ADDR_PID_KP = 6;     // 4 bytes
    static constexpr int EEPROM_ADDR_PID_KI = 10;    // 4 bytes
    static constexpr int EEPROM_ADDR_PID_KD = 14;    // 4 bytes
    static constexpr int EEPROM_ADDR_PID_MIN = 18;   // 4 bytes
    static constexpr int EEPROM_ADDR_PID_MAX = 22;   // 4 bytes
    static constexpr int EEPROM_ADDR_FAN_SPEED = 26;        // 4 bytes
    static constexpr int EEPROM_ADDR_SMART_ENABLED = 30;   // 1 byte
    static constexpr int EEPROM_ADDR_SMART_SETPOINT = 31;  // 4 bytes

    static constexpr float DEFAULT_SETPOINT = 0.0f;  // 0°C = 32°F
    static constexpr float DEFAULT_FAN_SPEED = 100.0f;
    static constexpr float DEFAULT_SMART_SETPOINT = 50.0f;
};

#endif
