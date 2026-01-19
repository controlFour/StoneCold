#include "SettingsManager.h"
#include <EEPROM.h>

SettingsManager& SettingsManager::getInstance() {
    static SettingsManager instance;
    return instance;
}

void SettingsManager::begin() {
    EEPROM.begin(EEPROM_SIZE);
    load();
}

void SettingsManager::save() {
    EEPROM.write(EEPROM_ADDR_TEMP_UNIT, static_cast<uint8_t>(_tempUnit));
    EEPROM.put(EEPROM_ADDR_SETPOINT, _setpoint);
    EEPROM.write(EEPROM_ADDR_PID_MODE, static_cast<uint8_t>(_pidMode));
    EEPROM.put(EEPROM_ADDR_PID_KP, _pidKp);
    EEPROM.put(EEPROM_ADDR_PID_KI, _pidKi);
    EEPROM.put(EEPROM_ADDR_PID_KD, _pidKd);
    EEPROM.put(EEPROM_ADDR_PID_MIN, _pidMinOutput);
    EEPROM.put(EEPROM_ADDR_PID_MAX, _pidMaxOutput);
    EEPROM.put(EEPROM_ADDR_FAN_SPEED, _fanSpeed);
    EEPROM.write(EEPROM_ADDR_SMART_ENABLED, _smartControlEnabled ? 1 : 0);
    EEPROM.put(EEPROM_ADDR_SMART_SETPOINT, _smartSetpoint);
    EEPROM.commit();
}

void SettingsManager::load() {
    uint8_t storedUnit = EEPROM.read(EEPROM_ADDR_TEMP_UNIT);
    if (storedUnit == CELSIUS || storedUnit == FAHRENHEIT) {
        _tempUnit = static_cast<TempUnit>(storedUnit);
    } else {
        _tempUnit = CELSIUS;
    }

    EEPROM.get(EEPROM_ADDR_SETPOINT, _setpoint);
    // Validate setpoint (check for NaN or unreasonable values)
    if (isnan(_setpoint) || _setpoint < -50.0f || _setpoint > 50.0f) {
        _setpoint = DEFAULT_SETPOINT;
    }

    // Load PID settings
    uint8_t storedPidMode = EEPROM.read(EEPROM_ADDR_PID_MODE);
    if (storedPidMode <= PID_AUTOTUNE) {
        _pidMode = static_cast<PIDMode>(storedPidMode);
    } else {
        _pidMode = PID_OFF;
    }
    // Don't start in auto-tune mode after reboot
    if (_pidMode == PID_AUTOTUNE) {
        _pidMode = PID_OFF;
    }

    EEPROM.get(EEPROM_ADDR_PID_KP, _pidKp);
    EEPROM.get(EEPROM_ADDR_PID_KI, _pidKi);
    EEPROM.get(EEPROM_ADDR_PID_KD, _pidKd);
    EEPROM.get(EEPROM_ADDR_PID_MIN, _pidMinOutput);
    EEPROM.get(EEPROM_ADDR_PID_MAX, _pidMaxOutput);

    // Validate PID values
    if (isnan(_pidKp) || _pidKp < 0.0f || _pidKp > 100.0f) _pidKp = 2.0f;
    if (isnan(_pidKi) || _pidKi < 0.0f || _pidKi > 100.0f) _pidKi = 0.1f;
    if (isnan(_pidKd) || _pidKd < 0.0f || _pidKd > 100.0f) _pidKd = 1.0f;
    if (isnan(_pidMinOutput) || _pidMinOutput < 0.0f || _pidMinOutput > 100.0f) _pidMinOutput = 0.0f;
    if (isnan(_pidMaxOutput) || _pidMaxOutput < 0.0f || _pidMaxOutput > 100.0f) _pidMaxOutput = 100.0f;

    // Load fan speed
    EEPROM.get(EEPROM_ADDR_FAN_SPEED, _fanSpeed);
    if (isnan(_fanSpeed) || _fanSpeed < 0.0f || _fanSpeed > 100.0f) _fanSpeed = DEFAULT_FAN_SPEED;

    // Load smart control settings
    uint8_t smartEnabled = EEPROM.read(EEPROM_ADDR_SMART_ENABLED);
    _smartControlEnabled = (smartEnabled == 1);
    EEPROM.get(EEPROM_ADDR_SMART_SETPOINT, _smartSetpoint);
    if (isnan(_smartSetpoint) || _smartSetpoint < 0.0f || _smartSetpoint > 100.0f) _smartSetpoint = DEFAULT_SMART_SETPOINT;
}

TempUnit SettingsManager::getTempUnit() const {
    return _tempUnit;
}

void SettingsManager::setTempUnit(TempUnit unit) {
    _tempUnit = unit;
}

void SettingsManager::toggleTempUnit() {
    _tempUnit = (_tempUnit == CELSIUS) ? FAHRENHEIT : CELSIUS;
    save();
}

float SettingsManager::getSetpoint() const {
    return _setpoint;
}

void SettingsManager::setSetpoint(float celsius) {
    _setpoint = celsius;
    save();
}

float SettingsManager::getFanSpeed() const {
    return _fanSpeed;
}

void SettingsManager::setFanSpeed(float percent) {
    _fanSpeed = percent;
    save();
}

bool SettingsManager::getSmartControlEnabled() const {
    return _smartControlEnabled;
}

void SettingsManager::setSmartControlEnabled(bool enabled) {
    _smartControlEnabled = enabled;
    save();
}

float SettingsManager::getSmartSetpoint() const {
    return _smartSetpoint;
}

void SettingsManager::setSmartSetpoint(float percent) {
    _smartSetpoint = percent;
    save();
}

PIDMode SettingsManager::getPIDMode() const {
    return _pidMode;
}

void SettingsManager::setPIDMode(PIDMode mode, bool saveNow) {
    _pidMode = mode;
    if (saveNow) save();
}

float SettingsManager::getPIDKp() const { return _pidKp; }
float SettingsManager::getPIDKi() const { return _pidKi; }
float SettingsManager::getPIDKd() const { return _pidKd; }

void SettingsManager::setPIDTunings(float kp, float ki, float kd, bool saveNow) {
    _pidKp = kp;
    _pidKi = ki;
    _pidKd = kd;
    if (saveNow) save();
}

float SettingsManager::getPIDMinOutput() const { return _pidMinOutput; }
float SettingsManager::getPIDMaxOutput() const { return _pidMaxOutput; }

void SettingsManager::setPIDOutputLimits(float min, float max, bool saveNow) {
    _pidMinOutput = min;
    _pidMaxOutput = max;
    if (saveNow) save();
}

float SettingsManager::toDisplayUnit(float celsius) const {
    return (_tempUnit == CELSIUS) ? celsius : celsiusToFahrenheit(celsius);
}

float SettingsManager::celsiusToFahrenheit(float celsius) const {
    return celsius * 9.0f / 5.0f + 32.0f;
}

float SettingsManager::fahrenheitToCelsius(float fahrenheit) const {
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;
}

const char* SettingsManager::getUnitString() const {
    return (_tempUnit == CELSIUS) ? "C" : "F";
}

const char* SettingsManager::getUnitSymbol() const {
    return (_tempUnit == CELSIUS) ? "C" : "F";
}
