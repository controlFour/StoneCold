#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <stdint.h>
#include <math.h>

class TemperatureSensor {
public:
    static TemperatureSensor& getInstance();

    void begin();
    float readTemperature();  // Returns temperature in Celsius, or NAN on error

    // Error handling
    bool hasError() const { return _hasError; }
    void tryReconnect();  // Attempt to reconnect if in error state

private:
    TemperatureSensor() = default;
    TemperatureSensor(const TemperatureSensor&) = delete;
    TemperatureSensor& operator=(const TemperatureSensor&) = delete;

    // Software SPI via PCA9554
    uint8_t softSPI_transfer(uint8_t data);

    // MAX31865 RTD sensor methods
    void max31865_init();
    void max31865_write(uint8_t reg, uint8_t value);
    uint8_t max31865_read(uint8_t reg);
    uint16_t max31865_readRTD();
    float rtdToTemperature(uint16_t rtd);

    // MAX31865 SPI pins on PCA9554
    static constexpr uint8_t PIN_CLK = 0;
    static constexpr uint8_t PIN_SDO = 1;  // MISO - data from MAX31865
    static constexpr uint8_t PIN_SDI = 2;  // MOSI - data to MAX31865
    static constexpr uint8_t PIN_CS = 3;

    // MAX31865 registers
    static constexpr uint8_t MAX31865_CONFIG_REG = 0x00;
    static constexpr uint8_t MAX31865_RTD_MSB = 0x01;
    static constexpr uint8_t MAX31865_RTD_LSB = 0x02;
    static constexpr uint8_t MAX31865_FAULT_STATUS = 0x07;

    // MAX31865 configuration bits
    static constexpr uint8_t MAX31865_CONFIG_BIAS = 0x80;
    static constexpr uint8_t MAX31865_CONFIG_1SHOT = 0x20;
    static constexpr uint8_t MAX31865_CONFIG_FAULT_CLEAR = 0x02;

    // PT100 RTD constants
    static constexpr float RTD_RREF = 430.0f;
    static constexpr float RTD_NOMINAL = 100.0f;
    static constexpr float RTD_A = 3.9083e-3f;
    static constexpr float RTD_B = -5.775e-7f;

    // Valid temperature range (outside this = error)
    static constexpr float TEMP_MIN_VALID = -50.0f;  // -58°F
    static constexpr float TEMP_MAX_VALID = 150.0f;  // 302°F

    // Error state
    bool _hasError = false;
};

#endif
