#ifndef TEC_CONTROLLER_H
#define TEC_CONTROLLER_H

#include <stdint.h>

class TECController {
public:
    static TECController& getInstance();

    void begin();
    void update();  // Call in loop() for soft-start ramping

    // Enable/disable the TEC
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    // Set cooling power (0.0 to 1.0)
    // If instant=true, bypasses soft-start ramp (use for auto-tune)
    void setPower(float power, bool instant = false);
    float getPower() const { return _power; }
    float getTargetPower() const { return _targetPower; }

    // Current sensing
    float readCurrent();  // Returns current in amps

    // Convenience methods
    void stop();  // Disable and set power to 0

private:
    TECController() = default;
    TECController(const TECController&) = delete;
    TECController& operator=(const TECController&) = delete;

    void updatePWM();

    bool _enabled = false;
    float _power = 0.0f;
    float _targetPower = 0.0f;

    // Hardware configuration
    static constexpr uint8_t PIN_RPWM = 2;       // GPIO2 on M5Dial Port B
    static constexpr uint8_t PIN_REN = 4;        // PCA9554 pin 4
    static constexpr uint8_t PIN_RIS = 1;        // GPIO1 - current sense (Port B)

    // LEDC PWM configuration
    static constexpr uint8_t PWM_CHANNEL = 0;
    static constexpr uint32_t PWM_FREQ = 20000;  // 20kHz (above human hearing)
    static constexpr uint8_t PWM_RESOLUTION = 10; // 10-bit (0-1023)

    // Soft-start configuration
    static constexpr float RAMP_RATE = 0.01f;    // 1% per update (~2-3 sec to full power)

    // Current sensing (BTS7960 IS pin)
    static constexpr float IS_MV_PER_AMP = 8.5f; // ~8.5mV per amp
};

#endif
