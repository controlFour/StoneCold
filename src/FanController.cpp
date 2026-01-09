#include "FanController.h"
#include "PCA9554.h"
#include <Arduino.h>

FanController& FanController::getInstance() {
    static FanController instance;
    return instance;
}

void FanController::begin() {
    auto& io = PCA9554::getInstance();

    if (!io.isOnline()) {
        _online = false;
        return;
    }

    // Configure tach pins as inputs
    io.setPinMode(PIN_FAN1_TACH, false);  // Input
    io.setPinMode(PIN_FAN2_TACH, false);  // Input

    // Configure PWM pin (firmware v3+)
    // Note: EXTIO2 max is 2kHz (mode 0), not ideal for PC fans that expect 25kHz
    io.setPWMPinMode(PIN_FAN_PWM);
    io.setPWMFrequency(0);  // 0=2kHz, 1=1kHz, 2=500Hz, 3=250Hz, 4=125Hz

    // Read initial states
    _fan1LastState = io.digitalRead(PIN_FAN1_TACH);
    _fan2LastState = io.digitalRead(PIN_FAN2_TACH);

    _fan1PulseCount = 0;
    _fan2PulseCount = 0;
    _lastCalcTime = millis();

    _online = true;

    // Set fans to 100% speed initially
    setSpeed(100);
}

void FanController::update() {
    if (!_online) return;

    auto& io = PCA9554::getInstance();
    if (!io.isOnline()) {
        _online = false;
        return;
    }

    // Read current tach states
    bool fan1State = io.digitalRead(PIN_FAN1_TACH);
    bool fan2State = io.digitalRead(PIN_FAN2_TACH);

    // Count rising edges (low to high transitions)
    if (fan1State && !_fan1LastState) {
        _fan1PulseCount++;
    }
    if (fan2State && !_fan2LastState) {
        _fan2PulseCount++;
    }

    _fan1LastState = fan1State;
    _fan2LastState = fan2State;

    // Calculate RPM periodically
    unsigned long now = millis();
    if (now - _lastCalcTime >= CALC_INTERVAL_MS) {
        calculateRPM();
    }
}

void FanController::calculateRPM() {
    unsigned long now = millis();
    unsigned long elapsed = now - _lastCalcTime;

    if (elapsed > 0) {
        // RPM = (pulses / PULSES_PER_REV) * (60000 / elapsed_ms)
        // Simplified: RPM = pulses * 60000 / (PULSES_PER_REV * elapsed)
        _fan1RPM = (uint16_t)((_fan1PulseCount * 60000UL) / (PULSES_PER_REV * elapsed));
        _fan2RPM = (uint16_t)((_fan2PulseCount * 60000UL) / (PULSES_PER_REV * elapsed));
    }

    _fan1PulseCount = 0;
    _fan2PulseCount = 0;
    _lastCalcTime = now;
}

uint16_t FanController::getAverageRPM() const {
    if (_fan1RPM > 0 && _fan2RPM > 0) {
        return (_fan1RPM + _fan2RPM) / 2;
    } else if (_fan1RPM > 0) {
        return _fan1RPM;
    } else {
        return _fan2RPM;
    }
}

void FanController::setSpeed(uint8_t percent) {
    if (!_online) return;
    if (percent > 100) percent = 100;

    _speedPercent = percent;

    auto& io = PCA9554::getInstance();
    io.setPWMDutyCycle(PIN_FAN_PWM, percent);
}
