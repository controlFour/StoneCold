#include "PIDController.h"
#include "SettingsManager.h"
#include <Arduino.h>

extern void logPrintf(const char* format, ...);

PIDController& PIDController::getInstance() {
    static PIDController instance;
    return instance;
}

void PIDController::begin() {
    // Load settings
    auto& settings = SettingsManager::getInstance();
    _kp = settings.getPIDKp();
    _ki = settings.getPIDKi();
    _kd = settings.getPIDKd();
    _minOutput = settings.getPIDMinOutput();
    _maxOutput = settings.getPIDMaxOutput();
    _mode = settings.getPIDMode();

    // Create PID controller
    _pid = new QuickPID(&_input, &_output, &_setpoint,
                        _kp, _ki, _kd,
                        QuickPID::pMode::pOnError,
                        QuickPID::dMode::dOnMeas,
                        QuickPID::iAwMode::iAwClamp,
                        QuickPID::Action::reverse);  // Reverse: higher output = more cooling = lower temp

    _pid->SetOutputLimits(_minOutput, _maxOutput);
    _pid->SetSampleTimeUs(SAMPLE_TIME_MS * 1000);

    if (_mode == PID_ON) {
        _pid->SetMode(QuickPID::Control::automatic);
    } else {
        _pid->SetMode(QuickPID::Control::manual);
    }
}

void PIDController::update(float currentTemp, float setpoint) {
    _input = currentTemp;

    // Don't overwrite setpoint during auto-tune (it uses a temporary one)
    if (!_autoTuning) {
        _setpoint = setpoint;
    }

    if (_autoTuning) {
        runAutoTune();
        return;
    }

    if (_mode == PID_ON && _pid) {
        _pid->Compute();
    } else if (_mode == PID_OFF) {
        _output = 0.0f;
    }
}

void PIDController::setMode(PIDMode mode, bool saveToEeprom) {
    // Stop auto-tune if switching away from it
    if (_autoTuning && mode != PID_AUTOTUNE) {
        stopAutoTune();
    }

    _mode = mode;

    if (_pid) {
        if (mode == PID_ON) {
            _pid->SetMode(QuickPID::Control::automatic);
        } else if (mode == PID_OFF) {
            _pid->SetMode(QuickPID::Control::manual);
            _output = 0.0f;
        } else if (mode == PID_AUTOTUNE) {
            startAutoTune();
        }
    }

    SettingsManager::getInstance().setPIDMode(mode, saveToEeprom);
}

void PIDController::setTunings(float kp, float ki, float kd, bool saveToEeprom) {
    _kp = kp;
    _ki = ki;
    _kd = kd;

    if (_pid) {
        _pid->SetTunings(_kp, _ki, _kd);
    }

    auto& settings = SettingsManager::getInstance();
    settings.setPIDTunings(_kp, _ki, _kd, saveToEeprom);
}

void PIDController::setOutputLimits(float min, float max, bool saveToEeprom) {
    _minOutput = min;
    _maxOutput = max;

    if (_pid) {
        _pid->SetOutputLimits(_minOutput, _maxOutput);
    }

    auto& settings = SettingsManager::getInstance();
    settings.setPIDOutputLimits(_minOutput, _maxOutput, saveToEeprom);
}

void PIDController::startAutoTune() {
    _autoTuning = true;
    _autoTuneComplete = false;
    _autoTuneStart = millis();
    _autoTuneCycles = 0;
    _autoTunePeakHigh = -1000.0f;
    _autoTunePeakLow = 1000.0f;
    _autoTuneHigh = true;
    _autoTuneOutput = _maxOutput;  // Start with cooling on
    _autoTuneLastCross = millis();
    _autoTunePeriodSum = 0.0f;
    _autoTuneAmplitudeSum = 0.0f;

    // Save original setpoint and set temporary one 3°F (~1.7°C) below current temp
    _savedSetpoint = _setpoint;
    float offset = 3.0f / 1.8f;  // 3°F in Celsius
    _setpoint = _input - offset;

    if (_pid) {
        _pid->SetMode(QuickPID::Control::manual);
    }

    float inputF = _input * 9.0f / 5.0f + 32.0f;
    float setpointF = _setpoint * 9.0f / 5.0f + 32.0f;
    float savedF = _savedSetpoint * 9.0f / 5.0f + 32.0f;
    logPrintf("Auto-tune started: current=%.1fF, temp setpoint=%.1fF (original=%.1fF)\n",
              inputF, setpointF, savedF);
}

void PIDController::stopAutoTune() {
    _autoTuning = false;
    _output = 0.0f;

    // Restore original setpoint
    _setpoint = _savedSetpoint;

    // Restore previous mode
    if (_mode == PID_ON && _pid) {
        _pid->SetMode(QuickPID::Control::automatic);
    }

    logPrintf("Auto-tune stopped, restored setpoint=%.1fC\n", _setpoint);
}

bool PIDController::checkAndClearAutoTuneComplete() {
    if (_autoTuneComplete) {
        _autoTuneComplete = false;
        return true;
    }
    return false;
}

void PIDController::runAutoTune() {
    // Debug mode: fake auto-tune completes after 3 seconds with test values
    if (DEBUG_FAKE_AUTOTUNE) {
        static unsigned long fakeStart = 0;
        if (fakeStart == 0) fakeStart = millis();

        logPrintf("Fake auto-tune: %lu ms elapsed\n", millis() - fakeStart);

        if (millis() - fakeStart > 3000) {
            // Simulate completion with reasonable PID values
            float newKp = 5.0f;
            float newKi = 0.5f;
            float newKd = 2.0f;

            logPrintf("Fake auto-tune complete: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", newKp, newKi, newKd);

            setTunings(newKp, newKi, newKd, false);
            _autoTuning = false;
            _autoTuneComplete = true;
            _setpoint = _savedSetpoint;
            _mode = PID_ON;
            SettingsManager::getInstance().setPIDMode(PID_ON, false);

            if (_pid) {
                _pid->SetMode(QuickPID::Control::automatic);
            }
            fakeStart = 0;  // Reset for next time
        }
        return;
    }

    // Relay auto-tune (bang-bang oscillation method)
    // Oscillate between min and max output, measure response

    // Debug output every 5 seconds
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 5000) {
        float inputF = _input * 9.0f / 5.0f + 32.0f;
        float setpointF = _setpoint * 9.0f / 5.0f + 32.0f;
        float peakHighF = _autoTunePeakHigh * 9.0f / 5.0f + 32.0f;
        float peakLowF = _autoTunePeakLow * 9.0f / 5.0f + 32.0f;
        logPrintf("AutoTune: temp=%.1fF, target=%.1fF, %s, power=%.0f%%, cycles=%d\n",
                  inputF, setpointF, _autoTuneHigh ? "COOLING" : "HEATING", _output, _autoTuneCycles / 2);
        logPrintf("  peaks: high=%.1fF, low=%.1fF, swing=%.1fF\n",
                  peakHighF, peakLowF, peakHighF - peakLowF);
        lastDebug = millis();
    }

    // Track peaks
    if (_input > _autoTunePeakHigh) _autoTunePeakHigh = _input;
    if (_input < _autoTunePeakLow) _autoTunePeakLow = _input;

    // Check for zero crossing (temperature crosses setpoint)
    bool shouldSwitch = false;
    if (_autoTuneHigh && _input < _setpoint) {
        // Was cooling, now below setpoint
        float inputF = _input * 9.0f / 5.0f + 32.0f;
        float setpointF = _setpoint * 9.0f / 5.0f + 32.0f;
        logPrintf("AutoTune: CROSSING! %.1fF < %.1fF, switching to HEATING\n", inputF, setpointF);
        shouldSwitch = true;
    } else if (!_autoTuneHigh && _input > _setpoint) {
        // Was off, now above setpoint
        float inputF = _input * 9.0f / 5.0f + 32.0f;
        float setpointF = _setpoint * 9.0f / 5.0f + 32.0f;
        logPrintf("AutoTune: CROSSING! %.1fF > %.1fF, switching to COOLING\n", inputF, setpointF);
        shouldSwitch = true;
    }

    if (shouldSwitch) {
        unsigned long now = millis();
        unsigned long period = now - _autoTuneLastCross;

        _autoTuneCycles++;

        if (_autoTuneCycles > 1 && period > 1000) {  // Ignore first crossing and short periods
            _autoTunePeriodSum += period;
            _autoTuneAmplitudeSum += (_autoTunePeakHigh - _autoTunePeakLow);

            logPrintf("Auto-tune: cycle %d/5, period=%.1fs, swing=%.1fF\n",
                      _autoTuneCycles / 2, period / 1000.0f,
                      (_autoTunePeakHigh - _autoTunePeakLow) * 9.0f / 5.0f);
        }
        _autoTuneLastCross = now;
        _autoTunePeakHigh = _input;
        _autoTunePeakLow = _input;
        _autoTuneHigh = !_autoTuneHigh;
    }

    // Set output based on current state
    _output = _autoTuneHigh ? _maxOutput : _minOutput;

    // Check if we have enough cycles
    if (_autoTuneCycles >= AUTOTUNE_CYCLES * 2) {  // *2 because we count half-cycles
        // Calculate tuning parameters using Ziegler-Nichols
        float avgPeriod = _autoTunePeriodSum / (AUTOTUNE_CYCLES - 1);  // ms
        float avgAmplitude = _autoTuneAmplitudeSum / (AUTOTUNE_CYCLES - 1);  // degrees

        // Protect against divide by zero
        if (avgAmplitude < 0.01f) avgAmplitude = 0.01f;
        if (avgPeriod < 100.0f) avgPeriod = 100.0f;

        float Tu = avgPeriod / 1000.0f;  // Ultimate period in seconds
        float Ku = (4.0f * (_maxOutput - _minOutput)) / (3.14159f * avgAmplitude);  // Ultimate gain

        // Conservative thermal system tuning
        // Based on "No Overshoot" but with reduced D term for noisy temp sensors
        float newKp = 0.2f * Ku;
        float newKi = 0.4f * Ku / Tu;
        float newKd = newKp * 0.25f;  // D = 25% of P (thermal-friendly ratio)

        logPrintf("Auto-tune complete: Tu=%.2fs, Ku=%.2f\n", Tu, Ku);
        logPrintf("Calculated (No Overshoot): Kp=%.2f, Ki=%.2f, Kd=%.2f\n", newKp, newKi, newKd);

        // Clamp to reasonable ranges to prevent NaN/Inf issues
        if (newKp < 0.0f || newKp > 50.0f || isnan(newKp) || isinf(newKp)) newKp = 2.0f;
        if (newKi < 0.0f || newKi > 10.0f || isnan(newKi) || isinf(newKi)) newKi = 0.1f;
        if (newKd < 0.0f || newKd > 50.0f || isnan(newKd) || isinf(newKd)) newKd = 1.0f;

        // Apply new tunings (don't save yet - user will save from menu)
        setTunings(newKp, newKi, newKd, false);

        _autoTuning = false;
        _autoTuneComplete = true;
        _setpoint = _savedSetpoint;  // Restore original setpoint
        _mode = PID_ON;
        SettingsManager::getInstance().setPIDMode(PID_ON, false);

        logPrintf("Restored setpoint=%.1fC\n", _setpoint);

        if (_pid) {
            _pid->SetMode(QuickPID::Control::automatic);
        }
    }

    // Timeout after 10 minutes
    if (millis() - _autoTuneStart > 600000) {
        logPrintf("Auto-tune timeout\n");
        stopAutoTune();
    }
}
