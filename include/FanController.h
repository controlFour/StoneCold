#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include <stdint.h>

class FanController {
public:
    static FanController& getInstance();

    void begin();
    void update();  // Call frequently to poll tach signals

    // Get RPM for each fan (0 if not spinning or not detected)
    uint16_t getFan1RPM() const { return _fan1RPM; }
    uint16_t getFan2RPM() const { return _fan2RPM; }

    // Get average RPM of both fans
    uint16_t getAverageRPM() const;

    // Fan speed control (0-100%)
    void setSpeed(uint8_t percent);
    uint8_t getSpeed() const { return _speedPercent; }

    bool isOnline() const { return _online; }

private:
    FanController() = default;
    FanController(const FanController&) = delete;
    FanController& operator=(const FanController&) = delete;

    void calculateRPM();

    // Fan tach pins on EXTIO2
    static constexpr uint8_t PIN_FAN1_TACH = 5;  // GPIO5
    static constexpr uint8_t PIN_FAN2_TACH = 6;  // GPIO6
    static constexpr uint8_t PIN_FAN_PWM = 7;    // GPIO7 - PWM control for both fans

    // Pulses per revolution (most PC fans = 2)
    static constexpr uint8_t PULSES_PER_REV = 2;

    // RPM calculation interval (ms)
    static constexpr unsigned long CALC_INTERVAL_MS = 500;

    // State tracking
    bool _online = false;
    bool _fan1LastState = false;
    bool _fan2LastState = false;
    uint16_t _fan1PulseCount = 0;
    uint16_t _fan2PulseCount = 0;
    uint16_t _fan1RPM = 0;
    uint16_t _fan2RPM = 0;
    unsigned long _lastCalcTime = 0;
    uint8_t _speedPercent = 100;
};

#endif
