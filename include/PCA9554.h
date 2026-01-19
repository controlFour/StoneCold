#ifndef PCA9554_H
#define PCA9554_H

#include <stdint.h>
#include <M5_EXTIO2.h>

// Wrapper class that provides the same interface as the old PCA9554
// but uses the M5Stack Extend IO 2 (STM32F030) internally
class PCA9554 {
public:
    static PCA9554& getInstance();

    void begin();

    // Pin configuration (call before using pins)
    void setPinMode(uint8_t pin, bool isOutput);  // true = output, false = input

    // Digital I/O
    void digitalWrite(uint8_t pin, bool level);
    bool digitalRead(uint8_t pin);

    // PWM/Servo control
    void setServoPinMode(uint8_t pin);
    void setServoAngle(uint8_t pin, uint8_t angle);  // 0-180

    // PWM mode (firmware v3+)
    void setPWMPinMode(uint8_t pin);
    void setPWMFrequency(uint8_t freqMode);  // 0=2kHz, 1=1kHz, 2=500Hz, 3=250Hz, 4=125Hz, 5=25kHz
    void setPWMDutyCycle(uint8_t pin, uint8_t percent);  // 0-100

    // FAN_RPM mode (custom firmware v5+)
    void setFanRPMPinMode(uint8_t pin);
    uint16_t readFanRPM(uint8_t pin);  // Returns RPM value (0 if not available)

    // Get current output state (for debugging)
    uint8_t getOutputState() const { return _outputState; }

    // Error handling
    bool isOnline() const { return _online; }
    void tryReconnect();  // Call periodically to attempt reconnection

private:
    PCA9554() = default;
    PCA9554(const PCA9554&) = delete;
    PCA9554& operator=(const PCA9554&) = delete;

    void recordError();
    void recordSuccess();

    M5_EXTIO2 _extio;
    uint8_t _outputState = 0xFF;  // Track output state for debugging

    // Error tracking
    bool _online = true;
    uint8_t _errorCount = 0;
    unsigned long _lastRetryTime = 0;

    // EXTIO2 I2C configuration
    static constexpr uint8_t I2C_ADDR = 0x45;  // Default EXTIO2 address

    // Error thresholds
    static constexpr uint8_t MAX_ERRORS = 5;
    static constexpr unsigned long RETRY_INTERVAL_MS = 5000;
};

#endif
