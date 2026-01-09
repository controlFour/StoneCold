#include "TECController.h"
#include "PCA9554.h"
#include <Arduino.h>

TECController& TECController::getInstance() {
    static TECController instance;
    return instance;
}

void TECController::begin() {
    auto& io = PCA9554::getInstance();

    // Configure REN pin as output on PCA9554
    io.setPinMode(PIN_REN, true);
    io.digitalWrite(PIN_REN, false);  // Start disabled

    // Configure LEDC PWM on RPWM pin (10kHz for TEC efficiency)
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_RPWM, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);

    _enabled = false;
    _power = 0.0f;
    _targetPower = 0.0f;

    // Configure current sense pin
    pinMode(PIN_RIS, INPUT);
}

void TECController::setEnabled(bool enabled) {
    if (_enabled == enabled) return;  // No change

    _enabled = enabled;

    auto& io = PCA9554::getInstance();
    io.digitalWrite(PIN_REN, enabled);

    if (!enabled) {
        // Disable immediately
        _power = 0.0f;
        _targetPower = 0.0f;
        ledcWrite(PWM_CHANNEL, 0);
    } else {
        // Soft-start: begin ramping from current power to target
        _power = 0.0f;  // Start from zero on enable
        updatePWM();
    }
}

void TECController::setPower(float power, bool instant) {
    // Clamp to valid range
    if (power < 0.0f) power = 0.0f;
    if (power > 1.0f) power = 1.0f;

    _targetPower = power;

    // Instant mode bypasses ramp (for auto-tune accuracy)
    if (instant) {
        _power = power;
        updatePWM();
    }
}

void TECController::update() {
    if (!_enabled) return;

    // Soft-start/soft-stop ramping
    if (_power < _targetPower) {
        _power += RAMP_RATE;
        if (_power > _targetPower) _power = _targetPower;
        updatePWM();
    } else if (_power > _targetPower) {
        _power -= RAMP_RATE;
        if (_power < _targetPower) _power = _targetPower;
        updatePWM();
    }
}

void TECController::stop() {
    _targetPower = 0.0f;
    _power = 0.0f;
    setEnabled(false);
}

void TECController::updatePWM() {
    uint32_t maxDuty = (1 << PWM_RESOLUTION) - 1;  // 1023 for 10-bit
    uint32_t duty = static_cast<uint32_t>(_power * maxDuty);
    ledcWrite(PWM_CHANNEL, duty);
}

float TECController::readCurrent() {
    int raw = analogRead(PIN_RIS);
    float voltage = raw * 3.3f / 4095.0f;

    // Debug: output raw ADC value
    Serial.printf("  [ADC raw=%d, voltage=%.3fV]\n", raw, voltage);

    // Calibrated: ~38mV per amp (0.33V at 8.7A)
    float current = voltage / 0.038f;

    return current;
}
