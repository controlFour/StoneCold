#include "TemperatureSensor.h"
#include "PCA9554.h"
#include <Arduino.h>
#include <math.h>

TemperatureSensor& TemperatureSensor::getInstance() {
    static TemperatureSensor instance;
    return instance;
}

void TemperatureSensor::begin() {
    auto& io = PCA9554::getInstance();

    // Check if I2C expander is online
    if (!io.isOnline()) {
        _hasError = true;
        return;
    }

    // Configure SPI pins on PCA9554
    // Add delays between I2C operations for EXTIO2 settling
    io.setPinMode(PIN_CLK, true);   // Output
    delay(5);
    io.setPinMode(PIN_SDO, false);  // Input (MISO)
    delay(5);
    io.setPinMode(PIN_SDI, true);   // Output
    delay(5);
    io.setPinMode(PIN_CS, true);    // Output
    delay(5);

    // Set initial state: CS high (inactive), CLK low
    io.digitalWrite(PIN_CS, true);
    io.digitalWrite(PIN_CLK, false);
    io.digitalWrite(PIN_SDI, false);
    delay(10);

    // Initialize MAX31865
    max31865_init();

    _hasError = !io.isOnline();
}

void TemperatureSensor::tryReconnect() {
    auto& io = PCA9554::getInstance();
    io.tryReconnect();

    if (io.isOnline() && _hasError) {
        // Try to reinitialize
        begin();
    }
}

float TemperatureSensor::readTemperature() {
    auto& io = PCA9554::getInstance();

    // Check if I2C expander is online
    if (!io.isOnline()) {
        _hasError = true;
        return NAN;
    }

    uint16_t rtd = max31865_readRTD();
    float temp = rtdToTemperature(rtd);

    // Validate temperature is in reasonable range
    if (temp < TEMP_MIN_VALID || temp > TEMP_MAX_VALID) {
        _hasError = true;
        return NAN;
    }

    _hasError = false;
    return temp;
}

// Software SPI (Mode 3: CPOL=1, CPHA=1)
uint8_t TemperatureSensor::softSPI_transfer(uint8_t data) {
    auto& io = PCA9554::getInstance();
    uint8_t received = 0;

    for (int i = 7; i >= 0; i--) {
        // Clock low (falling edge)
        io.digitalWrite(PIN_CLK, false);

        // Set MOSI (SDI)
        io.digitalWrite(PIN_SDI, (data >> i) & 0x01);
        delayMicroseconds(2);

        // Clock high (rising edge - sample MISO)
        io.digitalWrite(PIN_CLK, true);
        delayMicroseconds(2);

        // Read MISO (SDO) on rising edge
        if (io.digitalRead(PIN_SDO)) {
            received |= (1 << i);
        }
    }

    return received;
}

// MAX31865 methods

void TemperatureSensor::max31865_init() {
    // Clear any faults
    max31865_write(MAX31865_CONFIG_REG, MAX31865_CONFIG_FAULT_CLEAR);
    delay(10);
}

void TemperatureSensor::max31865_write(uint8_t reg, uint8_t value) {
    auto& io = PCA9554::getInstance();

    io.digitalWrite(PIN_CS, false);
    delayMicroseconds(10);
    softSPI_transfer(reg | 0x80);  // Set write bit
    softSPI_transfer(value);
    delayMicroseconds(10);
    io.digitalWrite(PIN_CS, true);
}

uint8_t TemperatureSensor::max31865_read(uint8_t reg) {
    auto& io = PCA9554::getInstance();

    io.digitalWrite(PIN_CS, false);
    delayMicroseconds(10);
    softSPI_transfer(reg & 0x7F);  // Clear write bit
    uint8_t value = softSPI_transfer(0xFF);
    delayMicroseconds(10);
    io.digitalWrite(PIN_CS, true);
    return value;
}

uint16_t TemperatureSensor::max31865_readRTD() {
    // Enable bias and start 1-shot conversion
    uint8_t config = MAX31865_CONFIG_BIAS | MAX31865_CONFIG_1SHOT;
    max31865_write(MAX31865_CONFIG_REG, config);

    // Wait for conversion (typical 52ms for 60Hz filter)
    delay(65);

    // Read RTD registers
    uint8_t msb = max31865_read(MAX31865_RTD_MSB);
    uint8_t lsb = max31865_read(MAX31865_RTD_LSB);

    // Combine and remove fault bit
    uint16_t rtd = (static_cast<uint16_t>(msb) << 8) | lsb;
    rtd >>= 1;  // Remove fault bit (LSB)

    // Turn off bias to reduce self-heating
    max31865_write(MAX31865_CONFIG_REG, 0x00);

    return rtd;
}

float TemperatureSensor::rtdToTemperature(uint16_t rtd) {
    float resistance = static_cast<float>(rtd) * RTD_RREF / 32768.0f;

    // Callendar-Van Dusen equation (simplified for T > 0C)
    float Z1 = -RTD_A;
    float Z2 = RTD_A * RTD_A - (4 * RTD_B);
    float Z3 = (4 * RTD_B) / RTD_NOMINAL;
    float Z4 = 2 * RTD_B;

    float temp = Z2 + (Z3 * resistance);
    temp = (sqrt(temp) + Z1) / Z4;

    // For temperatures below 0C, use simple linear approximation
    if (temp < 0) {
        temp = (resistance - RTD_NOMINAL) / (RTD_NOMINAL * RTD_A);
    }

    return temp;
}
