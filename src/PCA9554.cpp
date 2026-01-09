#include "PCA9554.h"
#include <Arduino.h>
#include <Wire.h>

PCA9554& PCA9554::getInstance() {
    static PCA9554 instance;
    return instance;
}

void PCA9554::begin() {
    _online = true;
    _errorCount = 0;
    _outputState = 0xFF;

    // Initialize EXTIO2 with Wire (already initialized in main.cpp)
    // SDA=13, SCL=15 for Port A on M5Dial
    if (!_extio.begin(&Wire, 13, 15, I2C_ADDR)) {
        recordError();
        return;
    }

    // Set all pins to digital input mode by default (safe state)
    if (!_extio.setAllPinMode(DIGITAL_INPUT_MODE)) {
        recordError();
    }
}

void PCA9554::setPinMode(uint8_t pin, bool isOutput) {
    if (pin > 7 || !_online) return;

    extio_io_mode_t mode = isOutput ? DIGITAL_OUTPUT_MODE : DIGITAL_INPUT_MODE;
    if (!_extio.setPinMode(pin, mode)) {
        recordError();
    } else {
        recordSuccess();
    }
}

void PCA9554::digitalWrite(uint8_t pin, bool level) {
    if (pin > 7 || !_online) return;

    if (level) {
        _outputState |= (1 << pin);
    } else {
        _outputState &= ~(1 << pin);
    }

    if (!_extio.setDigitalOutput(pin, level ? 1 : 0)) {
        recordError();
    } else {
        recordSuccess();
    }
}

bool PCA9554::digitalRead(uint8_t pin) {
    if (pin > 7 || !_online) return false;

    bool value = _extio.getDigitalInput(pin);
    recordSuccess();
    return value;
}

void PCA9554::setServoPinMode(uint8_t pin) {
    if (pin > 7 || !_online) return;

    if (!_extio.setPinMode(pin, SERVO_CTL_MODE)) {
        recordError();
    } else {
        recordSuccess();
    }
}

void PCA9554::setServoAngle(uint8_t pin, uint8_t angle) {
    if (pin > 7 || !_online) return;

    if (!_extio.setServoAngle(pin, angle)) {
        recordError();
    } else {
        recordSuccess();
    }
}

// PWM mode (firmware v3+) - direct I2C since not in library yet
static constexpr uint8_t PWM_IO_MODE = 5;
static constexpr uint8_t REG_MODE_BASE = 0x00;
static constexpr uint8_t REG_PWM_DUTY_BASE = 0x90;
static constexpr uint8_t REG_PWM_FREQ = 0xA0;

void PCA9554::setPWMPinMode(uint8_t pin) {
    if (pin > 7 || !_online) return;

    Wire.beginTransmission(I2C_ADDR);
    Wire.write(REG_MODE_BASE + pin);
    Wire.write(PWM_IO_MODE);
    if (Wire.endTransmission() == 0) {
        recordSuccess();
    } else {
        recordError();
    }
}

void PCA9554::setPWMFrequency(uint8_t freqMode) {
    if (!_online) return;
    if (freqMode > 4) freqMode = 4;

    Wire.beginTransmission(I2C_ADDR);
    Wire.write(REG_PWM_FREQ);
    Wire.write(freqMode);
    if (Wire.endTransmission() == 0) {
        recordSuccess();
    } else {
        recordError();
    }
}

void PCA9554::setPWMDutyCycle(uint8_t pin, uint8_t percent) {
    if (pin > 7 || !_online) return;
    if (percent > 100) percent = 100;

    // Duty cycle: 1 byte per channel at base + pin
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(REG_PWM_DUTY_BASE + pin);
    Wire.write(percent);
    if (Wire.endTransmission() == 0) {
        recordSuccess();
    } else {
        recordError();
    }
}

void PCA9554::tryReconnect() {
    if (_online) return;

    unsigned long now = millis();
    if (now - _lastRetryTime < RETRY_INTERVAL_MS) return;

    _lastRetryTime = now;

    // Try to reinitialize
    if (_extio.begin(&Wire, 13, 15, I2C_ADDR)) {
        _online = true;
        _errorCount = 0;
        // Reinitialize all pins to input mode
        _extio.setAllPinMode(DIGITAL_INPUT_MODE);
    }
}

void PCA9554::recordError() {
    _errorCount++;
    if (_errorCount >= MAX_ERRORS) {
        _online = false;
    }
}

void PCA9554::recordSuccess() {
    _errorCount = 0;
}
