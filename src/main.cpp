#include <M5Dial.h>
#include <Wire.h>

// Set to 0 to disable WiFi, OTA, and telnet
#define ENABLE_WIFI 0  

#if ENABLE_WIFI
#include <WiFi.h>
#include <ArduinoOTA.h>

// WiFi credentials - UPDATE THESE
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";


// Telnet server for wireless serial monitor
WiFiServer telnetServer(23);
WiFiClient telnetClient;
#endif

#include "PCA9554.h"

// Printf to both Serial and Telnet
void logPrintf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    Serial.print(buf);
#if ENABLE_WIFI
    if (telnetClient && telnetClient.connected()) {
        telnetClient.print(buf);
    }
#endif
}

#include "SettingsManager.h"
#include "TemperatureSensor.h"
#include "TECController.h"
#include "FanController.h"
#include "InputController.h"
#include "DisplayManager.h"
#include "UIStateMachine.h"
#include "PIDController.h"

extern "C" {
    #include "snow_effect.h"
}

// Snow effect state
static bool snowInitialized = false;

// Current averaging - filter out bad ADC readings (PWM off-phase samples)
static constexpr int CURRENT_AVG_SIZE = 10;
static constexpr float CURRENT_MIN_THRESHOLD = 1.0f;  // Readings below this are considered bad
static float currentBuffer[CURRENT_AVG_SIZE] = {0};
static int currentBufferIdx = 0;
static int currentBufferCount = 0;

static void addCurrentReading(float current) {
    currentBuffer[currentBufferIdx] = current;
    currentBufferIdx = (currentBufferIdx + 1) % CURRENT_AVG_SIZE;
    if (currentBufferCount < CURRENT_AVG_SIZE) currentBufferCount++;
}

static float getFilteredAverageCurrent() {
    float sum = 0;
    int validCount = 0;
    for (int i = 0; i < currentBufferCount; i++) {
        if (currentBuffer[i] >= CURRENT_MIN_THRESHOLD) {
            sum += currentBuffer[i];
            validCount++;
        }
    }
    return (validCount > 0) ? (sum / validCount) : 0.0f;
}

void setup() {
    Serial.begin(115200);

    // Initialize M5Stack Dial
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

    // Release M5Dial's external I2C and reinitialize Wire for Port A
    M5.Ex_I2C.release();
    Wire.begin(13, 15);  // Port A: SDA=GPIO13, SCL=GPIO15

    // I2C scan for debugging
    Serial.println("I2C Scan:");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Found device at 0x%02X", addr);
            if (addr == 0x45) Serial.print(" (EXTIO2 app)");
            else if (addr == 0x54) Serial.print(" (EXTIO2 bootloader)");
            Serial.println();
        }
    }
    Serial.println("I2C Scan complete");

    // Initialize all subsystems
    SettingsManager::getInstance().begin();
    PCA9554::getInstance().begin();           // Must be before TemperatureSensor and TECController
    delay(100);  // Let EXTIO2 fully initialize before configuring pins
    TemperatureSensor::getInstance().begin();
    TECController::getInstance().begin();
    FanController::getInstance().begin();
    InputController::getInstance().begin();
    DisplayManager::getInstance().begin();
    UIStateMachine::getInstance().begin();

    // Initialize snow effect
    snow_effect_init(DisplayManager::getInstance().getActiveScreen());
    snowInitialized = true;

    // Initialize PID controller
    PIDController::getInstance().begin();

    // Enable TEC (power will be controlled by PID)
    TECController::getInstance().setEnabled(true);

    // Give EXTIO2 time to initialize FAN_RPM and PWM modes
    // Serial console connection adds delay; without it, EXTIO2 isn't ready
    delay(500);

#if ENABLE_WIFI
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

        ArduinoOTA.setHostname("stonecold");
        ArduinoOTA.begin();
        Serial.println("ArduinoOTA ready");

        telnetServer.begin();
        telnetServer.setNoDelay(true);
        Serial.println("Telnet server ready on port 23");
    } else {
        Serial.println("\nWiFi failed - OTA disabled");
    }
#endif
}

void loop() {
    M5Dial.update();

#if ENABLE_WIFI
    ArduinoOTA.handle();

    // Handle telnet client connections
    if (telnetServer.hasClient()) {
        if (telnetClient && telnetClient.connected()) {
            telnetClient.stop();  // Disconnect old client
        }
        telnetClient = telnetServer.available();
        telnetClient.println("Connected to Stonecold");
    }
#endif

    auto& ui = UIStateMachine::getInstance();
    auto& display = DisplayManager::getInstance();
    auto& input = InputController::getInstance();

    // Deinit snow BEFORE processing input that could trigger screen transitions
    // (screen transitions delete the main screen which snow is attached to)
    if (snowInitialized && input.wasButtonPressed()) {
        snow_effect_deinit();
        snowInitialized = false;
        // Re-queue the button press so UI can handle it
        input.requeueButtonPress();
    }

    // Update input and UI state
    input.update();
    ui.update();

    // Update TEC soft-start ramping
    auto& tec = TECController::getInstance();
    tec.update();

    // Update fan RPM readings
    FanController::getInstance().update();

    // Check for sensor errors and try to reconnect periodically
    auto& tempSensor = TemperatureSensor::getInstance();
    if (tempSensor.hasError()) {
        tempSensor.tryReconnect();
    }

    // Update PID controller and set TEC power (only if sensor is working)
    auto& pid = PIDController::getInstance();
    float currentTemp = tempSensor.readTemperature();
    bool sensorError = isnan(currentTemp);

    if (!sensorError) {
        pid.update(currentTemp, ui.getSetpoint());
        // Use instant power changes during auto-tune for accurate measurements
        tec.setPower(pid.getOutput(), pid.isAutoTuning());
    } else {
        // Sensor error - disable TEC for safety
        tec.setPower(0.0f, false);
    }

    // Smart fan control
    auto& fans = FanController::getInstance();
    auto& settings = SettingsManager::getInstance();

    // Track setpoint changes to ramp fans when user adjusts setpoint
    static float lastSetpoint = NAN;
    static bool waitingForSetpoint = false;

    if (settings.getSmartControlEnabled() && !sensorError) {
        float setpointC = ui.getSetpoint();
        static constexpr float HYSTERESIS_C = 2.78f;  // ~5°F

        // Detect setpoint change - ramp fans to max until new setpoint reached
        if (!isnan(lastSetpoint) && setpointC != lastSetpoint) {
            waitingForSetpoint = true;
            fans.setSpeed(static_cast<uint8_t>(settings.getFanSpeed()));
        }
        lastSetpoint = setpointC;

        if (waitingForSetpoint) {
            // Waiting for new setpoint to be reached - keep fans at max
            if (currentTemp <= setpointC) {
                // Reached new setpoint - switch to smart mode
                waitingForSetpoint = false;
                fans.setSpeed(static_cast<uint8_t>(settings.getSmartSetpoint()));
            }
            // else keep fans at max (already set above or from previous iteration)
        } else if (currentTemp > setpointC + HYSTERESIS_C) {
            // Too warm - run at max fan speed
            fans.setSpeed(static_cast<uint8_t>(settings.getFanSpeed()));
        } else if (currentTemp <= setpointC) {
            // At or below setpoint - use smart setpoint (lower speed)
            fans.setSpeed(static_cast<uint8_t>(settings.getSmartSetpoint()));
        }
        // Between setpoint and setpoint+hysteresis: maintain current speed (no change)
    } else {
        // Smart mode off or sensor error - use max fan speed
        fans.setSpeed(static_cast<uint8_t>(settings.getFanSpeed()));
        lastSetpoint = ui.getSetpoint();  // Keep tracking even when disabled
        waitingForSetpoint = false;
    }

    // Log temperature and TEC current every second
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 1000) {
        if (sensorError) {
            logPrintf("Temp: ERROR | TEC: disabled\n");
        } else {
            float tempF = currentTemp * 9.0f / 5.0f + 32.0f;
            float setpointC = ui.getSetpoint();
            float setpointF = setpointC * 9.0f / 5.0f + 32.0f;
            float current = tec.readCurrent();
            addCurrentReading(current);
            float avgCurrent = getFilteredAverageCurrent();
            logPrintf("Temp: %.1fF (SP: %.1fF) | TEC: %.2fA (avg: %.2fA) | power: %.0f%% | LFan: %drpm RFan: %drpm\n",
                      tempF, setpointF, current, avgCurrent, tec.getPower() * 100.0f,
                      fans.getFan1RPM(), fans.getFan2RPM());
        }
        lastLog = millis();
    }

    // Update current screen if visible (use filtered average)
    if (display.isCurrentScreenVisible()) {
        display.updateCurrentScreen(getFilteredAverageCurrent());
    }

    // Update power screen if visible
    if (display.isPowerScreenVisible()) {
        display.updatePowerScreen(tec.getPower() * 100.0f);
    }

    // Fan screen is updated by UIStateMachine (RPM will be added when fan hardware is connected)

    // Snow effect conditions with hysteresis
    // Show snow when actively cooling and temp is above setpoint
    // Stop snow when temp gets close to setpoint (within 0.5°F)
    // Resume snow when temp drifts above setpoint + 2.0°F
    bool onMainScreen = !display.isSettingsScreenVisible() &&
                        !display.isPIDScreenVisible() &&
                        !display.isAutoTuneScreenVisible() &&
                        !display.isCurrentScreenVisible() &&
                        !display.isSetpointScreenVisible() &&
                        !display.isPowerScreenVisible() &&
                        !display.isFanScreenVisible() &&
                        !display.isFanSpeedScreenVisible() &&
                        !display.isSmartControlScreenVisible();
    bool isCooling = tec.getPower() > 0.0f;

    // Hysteresis for snow effect based on temperature error
    // Thresholds in Celsius: 0.5°F = 0.278°C, 2.0°F = 1.111°C
    static constexpr float SNOW_STOP_THRESHOLD_C = 0.278f;   // Stop snow when within 0.5°F of setpoint
    static constexpr float SNOW_START_THRESHOLD_C = 1.111f;  // Start snow when 2.0°F above setpoint
    static bool snowTempActive = true;  // Start active since we're likely cooling from warm

    if (!sensorError) {
        float setpointC = ui.getSetpoint();
        float errorC = currentTemp - setpointC;

        if (snowTempActive && errorC < SNOW_STOP_THRESHOLD_C) {
            snowTempActive = false;  // Reached setpoint - stop snow
        } else if (!snowTempActive && errorC > SNOW_START_THRESHOLD_C) {
            snowTempActive = true;   // Drifted away - resume snow
        }
    }

    bool shouldShowSnow = onMainScreen && ui.isInactive() && isCooling && snowTempActive;

    // Hide snow if user interacted or cooling stopped
    if (!shouldShowSnow && snowInitialized) {
        snow_effect_deinit();
        snowInitialized = false;
    }

    // Update display
    display.update();

    // Update snow effect (~33 FPS)
    static unsigned long lastSnowUpdate = 0;
    if (snowInitialized && millis() - lastSnowUpdate > 30) {
        snow_effect_manual_update();
        lastSnowUpdate = millis();
    }

    // Show snow when conditions are met
    if (shouldShowSnow && !snowInitialized) {
        snow_effect_init(display.getActiveScreen());
        snowInitialized = true;
    }

    delay(1);
}
