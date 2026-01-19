/**
 * @file EXTIO2Flasher.cpp
 * @brief I2C IAP implementation for EXTIO2 GPIO expander
 *
 * Ported from CoreInk IAP implementation.
 */

#include "EXTIO2Flasher.h"

bool EXTIO2Flasher::i2cDevicePresent(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

uint8_t EXTIO2Flasher::readVersion() {
    if (!i2cDevicePresent(APP_ADDR)) {
        return 0;
    }

    Wire.beginTransmission(APP_ADDR);
    Wire.write(CMD_VERSION);
    Wire.endTransmission(false);

    Wire.requestFrom(APP_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

bool EXTIO2Flasher::isDevicePresent() {
    return i2cDevicePresent(APP_ADDR);
}

bool EXTIO2Flasher::enterBootloader() {
    Serial.println("EXTIO2: Entering bootloader mode...");

    // Check if app is running
    if (!i2cDevicePresent(APP_ADDR)) {
        Serial.println("EXTIO2: App not responding, checking bootloader...");
        if (i2cDevicePresent(BOOTLOADER_ADDR)) {
            Serial.println("EXTIO2: Already in bootloader mode");
            return true;
        }
        _lastError = "Device not found";
        Serial.println("EXTIO2: Device not found!");
        return false;
    }

    // Send IAP mode command to app
    Wire.beginTransmission(APP_ADDR);
    Wire.write(CMD_IAP_MODE);
    Wire.write(0x01);
    Wire.endTransmission();

    // Wait for reset
    delay(100);

    // Check bootloader is responding
    for (int i = 0; i < 20; i++) {
        if (i2cDevicePresent(BOOTLOADER_ADDR)) {
            Serial.println("EXTIO2: Bootloader ready");
            return true;
        }
        delay(20);
    }

    _lastError = "Failed to enter bootloader";
    Serial.println("EXTIO2: Failed to enter bootloader");
    return false;
}

bool EXTIO2Flasher::flashPage(uint32_t address, const uint8_t* data) {
    Serial.printf("EXTIO2: Flashing page at 0x%08X...\n", address);

    // Build packet: [cmd(1), addr(4), len(2), reserved(1), data(1024)] = 1032 bytes
    Wire.beginTransmission(BOOTLOADER_ADDR);

    // Command
    Wire.write(IAP_CMD_WRITE);

    // Address (big-endian)
    Wire.write((address >> 24) & 0xFF);
    Wire.write((address >> 16) & 0xFF);
    Wire.write((address >> 8) & 0xFF);
    Wire.write(address & 0xFF);

    // Length (big-endian) - always 1024
    Wire.write((FLASH_PAGE_SIZE >> 8) & 0xFF);
    Wire.write(FLASH_PAGE_SIZE & 0xFF);

    // Reserved
    Wire.write(0x00);

    // Data (1024 bytes)
    Wire.write(data, FLASH_PAGE_SIZE);

    uint8_t result = Wire.endTransmission();
    if (result != 0) {
        Serial.printf("EXTIO2: I2C error: %d\n", result);
        _lastError = "I2C write error";
        return false;
    }

    // Wait for flash erase + programming
    delay(60);

    return true;
}

void EXTIO2Flasher::jumpToApp() {
    Serial.println("EXTIO2: Jumping to application...");

    Wire.beginTransmission(BOOTLOADER_ADDR);
    Wire.write(IAP_CMD_JUMP);
    Wire.endTransmission();

    delay(100);
}

bool EXTIO2Flasher::flashFirmware(const uint8_t* firmware, uint32_t len,
                                   std::function<void(int, int)> progressCallback) {
    Serial.printf("EXTIO2: Flashing firmware (%d bytes)...\n", len);

    if (len > FIRMWARE_MAX_SIZE) {
        _lastError = "Firmware too large";
        Serial.println("EXTIO2: Firmware too large!");
        return false;
    }

    // Enter bootloader
    if (!enterBootloader()) {
        return false;
    }

    uint32_t address = FLASH_START_ADDR;
    uint32_t offset = 0;
    uint32_t pageNum = 0;
    uint32_t totalPages = (len + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;

    while (offset < len) {
        uint16_t pageLen = min((uint32_t)FLASH_PAGE_SIZE, len - offset);

        // Pad last page with 0xFF if needed
        uint8_t pageBuffer[FLASH_PAGE_SIZE];
        memset(pageBuffer, 0xFF, FLASH_PAGE_SIZE);
        memcpy(pageBuffer, firmware + offset, pageLen);

        Serial.printf("EXTIO2: Page %d/%d\n", pageNum + 1, totalPages);

        // Call progress callback if provided
        if (progressCallback) {
            progressCallback(pageNum + 1, totalPages);
        }

        if (!flashPage(address, pageBuffer)) {
            Serial.printf("EXTIO2: Failed to flash page at 0x%08X\n", address);
            return false;
        }

        address += FLASH_PAGE_SIZE;
        offset += FLASH_PAGE_SIZE;
        pageNum++;
    }

    // Jump to application
    jumpToApp();

    // Verify app is responding
    delay(500);
    if (i2cDevicePresent(APP_ADDR)) {
        uint8_t version = readVersion();
        Serial.printf("EXTIO2: Flash complete! Version: %d\n", version);
        _lastError = "";
        return true;
    } else {
        _lastError = "App not responding after flash";
        Serial.println("EXTIO2: Warning - App not responding after flash");
        return false;
    }
}
