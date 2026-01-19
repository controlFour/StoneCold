#include "FanController.h"
#include "PCA9554.h"
#include <Arduino.h>

FanController& FanController::getInstance() {
    static FanController instance;
    return instance;
}

void FanController::begin() {
    auto& io = PCA9554::getInstance();

    Serial.println("FanController::begin()");

    if (!io.isOnline()) {
        Serial.println("  EXTIO2 offline!");
        _online = false;
        return;
    }

    Serial.println("  Setting FAN_RPM mode on pins 5,6");
    // Configure tach pins for FAN_RPM mode (hardware RPM counting)
    // Each mode change needs settling time for EXTIO2 firmware
    io.setFanRPMPinMode(PIN_FAN1_TACH);
    delay(10);
    io.setFanRPMPinMode(PIN_FAN2_TACH);
    delay(10);

    Serial.println("  Setting PWM mode on pin 7, freq=1 (1kHz)");
    // Configure PWM pin for fan speed control
    // Using 1kHz - software PWM can't reliably do 25kHz without starving I2C
    io.setPWMPinMode(PIN_FAN_PWM);
    delay(10);
    io.setPWMFrequency(1);  // 1=1kHz
    delay(10);

    _lastReadTime = millis();
    _online = true;

    // Set fans to 100% speed initially
    setSpeed(100);
    delay(50);  // Let PWM stabilize
    Serial.println("  FanController ready");
}

void FanController::update() {
    if (!_online) return;

    auto& io = PCA9554::getInstance();
    if (!io.isOnline()) {
        _online = false;
        return;
    }

    // Read RPM periodically
    unsigned long now = millis();
    if (now - _lastReadTime >= READ_INTERVAL_MS) {
        _fan1RPM = io.readFanRPM(PIN_FAN1_TACH);
        _fan2RPM = io.readFanRPM(PIN_FAN2_TACH);
        _lastReadTime = now;
    }
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
