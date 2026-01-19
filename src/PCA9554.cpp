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

    Serial.println("PCA9554::begin()");

    // Initialize EXTIO2 with Wire (already initialized in main.cpp)
    // SDA=13, SCL=15 for Port A on M5Dial
    if (!_extio.begin(&Wire, 13, 15, I2C_ADDR)) {
        Serial.println("  EXTIO2 begin failed!");
        recordError();
        return;
    }

    // Read firmware version
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0xFE);  // Version register
    if (Wire.endTransmission(false) == 0) {
        Wire.requestFrom(I2C_ADDR, (uint8_t)1);
        if (Wire.available()) {
            uint8_t version = Wire.read();
            Serial.printf("  EXTIO2 firmware version: %d\n", version);
        }
    }

    // Set all pins to digital input mode by default (safe state)
    if (!_extio.setAllPinMode(DIGITAL_INPUT_MODE)) {
        Serial.println("  setAllPinMode failed!");
        recordError();
    }

    Serial.println("  PCA9554 ready");
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

// Custom modes (firmware v3+) - direct I2C since not in library yet
static constexpr uint8_t PWM_IO_MODE = 5;
static constexpr uint8_t FAN_RPM_MODE = 6;  // Custom firmware mode for fan tach
static constexpr uint8_t REG_MODE_BASE = 0x00;
static constexpr uint8_t REG_PWM_DUTY_BASE = 0x90;
static constexpr uint8_t REG_PWM_FREQ = 0xA0;
static constexpr uint8_t REG_FAN_RPM_BASE = 0xB0;  // 2 bytes per channel, little-endian

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
    if (freqMode > 5) freqMode = 5;  // 0-4 standard, 5=25kHz (custom firmware)

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

void PCA9554::setFanRPMPinMode(uint8_t pin) {
    if (pin > 7 || !_online) return;

    Wire.beginTransmission(I2C_ADDR);
    Wire.write(REG_MODE_BASE + pin);
    Wire.write(FAN_RPM_MODE);
    if (Wire.endTransmission() == 0) {
        recordSuccess();
    } else {
        recordError();
    }
}

uint16_t PCA9554::readFanRPM(uint8_t pin) {
    if (pin > 7 || !_online) return 0;

    // Request 2 bytes from the RPM register (0xB0 + pin*2, little-endian)
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(REG_FAN_RPM_BASE + (pin * 2));
    if (Wire.endTransmission(false) != 0) {
        recordError();
        return 0;
    }

    Wire.requestFrom(I2C_ADDR, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint8_t lowByte = Wire.read();   // Little-endian: low byte first
        uint8_t highByte = Wire.read();
        recordSuccess();
        return (highByte << 8) | lowByte;
    }

    recordError();
    return 0;
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
