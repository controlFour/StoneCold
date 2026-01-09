#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <QuickPID.h>
#include "SettingsManager.h"  // For PIDMode enum

class PIDController {
public:
    static PIDController& getInstance();

    void begin();
    void update(float currentTemp, float setpoint);  // Call every loop

    // Get computed output (0.0 to 1.0)
    float getOutput() const { return _output / 100.0f; }

    // Mode control
    void setMode(PIDMode mode, bool saveToEeprom = false);
    PIDMode getMode() const { return _mode; }

    // Tuning parameters
    void setTunings(float kp, float ki, float kd, bool saveToEeprom = false);
    float getKp() const { return _kp; }
    float getKi() const { return _ki; }
    float getKd() const { return _kd; }

    // Output limits (0-100)
    void setOutputLimits(float min, float max, bool saveToEeprom = false);
    float getMinOutput() const { return _minOutput; }
    float getMaxOutput() const { return _maxOutput; }

    // Auto-tune status
    bool isAutoTuning() const { return _autoTuning; }
    bool isAutoTuneCooling() const { return _autoTuneHigh; }  // True when outputting max (cooling)
    bool checkAndClearAutoTuneComplete();  // Returns true once when auto-tune completes
    int getAutoTuneCycle() const { return _autoTuneCycles / 2; }  // Full cycles (not half-cycles)
    void startAutoTune();
    void stopAutoTune();

private:
    PIDController() = default;
    PIDController(const PIDController&) = delete;
    PIDController& operator=(const PIDController&) = delete;

    void runAutoTune();

    // PID variables (QuickPID uses floats)
    float _input = 0.0f;
    float _output = 0.0f;
    float _setpoint = 0.0f;

    // Tuning parameters
    float _kp = 2.0f;
    float _ki = 0.1f;
    float _kd = 1.0f;

    // Output limits (percentage)
    float _minOutput = 0.0f;
    float _maxOutput = 100.0f;

    PIDMode _mode = PID_OFF;

    // Auto-tune state
    bool _autoTuning = false;
    bool _autoTuneComplete = false;
    unsigned long _autoTuneStart = 0;
    float _autoTuneOutput = 0.0f;
    bool _autoTuneHigh = true;
    float _autoTunePeakHigh = -1000.0f;
    float _autoTunePeakLow = 1000.0f;
    int _autoTuneCycles = 0;
    unsigned long _autoTuneLastCross = 0;
    float _autoTunePeriodSum = 0.0f;
    float _autoTuneAmplitudeSum = 0.0f;
    float _savedSetpoint = 0.0f;  // Original setpoint to restore after auto-tune

    QuickPID* _pid = nullptr;

    static constexpr int SAMPLE_TIME_MS = 500;
    static constexpr int AUTOTUNE_CYCLES = 5;

    // Debug mode: set to true for quick fake auto-tune (3 seconds instead of full run)
    static constexpr bool DEBUG_FAKE_AUTOTUNE = false;
};

#endif
