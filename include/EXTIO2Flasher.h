/**
 * @file EXTIO2Flasher.h
 * @brief I2C IAP (In-Application Programming) for EXTIO2 GPIO expander
 *
 * Ported from CoreInk IAP implementation. Allows flashing firmware to
 * the STM32F030 on the EXTIO2 unit via I2C bootloader protocol.
 */

#ifndef EXTIO2_FLASHER_H
#define EXTIO2_FLASHER_H

#include <Arduino.h>
#include <Wire.h>
#include <functional>

class EXTIO2Flasher {
public:
    // Singleton access
    static EXTIO2Flasher& getInstance() {
        static EXTIO2Flasher instance;
        return instance;
    }

    // Delete copy/move constructors
    EXTIO2Flasher(const EXTIO2Flasher&) = delete;
    EXTIO2Flasher& operator=(const EXTIO2Flasher&) = delete;

    /**
     * @brief Read the current firmware version from EXTIO2
     * @return Firmware version (0 if device not found)
     */
    uint8_t readVersion();

    /**
     * @brief Check if EXTIO2 application is responding
     * @return true if device found at app address
     */
    bool isDevicePresent();

    /**
     * @brief Flash firmware to EXTIO2
     * @param firmware Pointer to firmware binary data
     * @param len Length of firmware in bytes
     * @param progressCallback Optional callback for progress updates (currentPage, totalPages)
     * @return true on success, false on failure
     */
    bool flashFirmware(const uint8_t* firmware, uint32_t len,
                       std::function<void(int, int)> progressCallback = nullptr);

    /**
     * @brief Get the last error message
     * @return Error description string
     */
    const char* getLastError() const { return _lastError; }

private:
    EXTIO2Flasher() : _lastError("") {}

    // I2C addresses
    static constexpr uint8_t APP_ADDR = 0x45;
    static constexpr uint8_t BOOTLOADER_ADDR = 0x54;

    // Commands
    static constexpr uint8_t CMD_IAP_MODE = 0xFD;
    static constexpr uint8_t CMD_VERSION = 0xFE;
    static constexpr uint8_t IAP_CMD_WRITE = 0x06;
    static constexpr uint8_t IAP_CMD_JUMP = 0x77;

    // Flash parameters
    static constexpr uint32_t FLASH_PAGE_SIZE = 1024;
    static constexpr uint32_t FLASH_START_ADDR = 0x08001000;
    static constexpr uint32_t FIRMWARE_MAX_SIZE = 0x2C00;  // 11264 bytes

    /**
     * @brief Enter bootloader mode
     * @return true if bootloader is ready
     */
    bool enterBootloader();

    /**
     * @brief Flash a single page
     * @param address Flash address
     * @param data Page data (1024 bytes)
     * @return true on success
     */
    bool flashPage(uint32_t address, const uint8_t* data);

    /**
     * @brief Jump from bootloader to application
     */
    void jumpToApp();

    /**
     * @brief Check if device is present at address
     */
    bool i2cDevicePresent(uint8_t addr);

    const char* _lastError;
};

#endif // EXTIO2_FLASHER_H
