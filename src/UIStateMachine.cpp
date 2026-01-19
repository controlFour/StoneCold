#include "UIStateMachine.h"
#include "SettingsManager.h"
#include "TemperatureSensor.h"
#include "FanController.h"
#include "InputController.h"
#include "PIDController.h"
#include "EXTIO2Flasher.h"
#include "firmware_custom.h"
#include "firmware_original.h"
#include <Arduino.h>

UIStateMachine& UIStateMachine::getInstance() {
    static UIStateMachine instance;
    return instance;
}

void UIStateMachine::begin() {
    _mode = MODE_NAVIGATE;
    _mainSelection = MAIN_SELECT_SETPOINT;
    _settingsSelection = SETTINGS_TEMP_UNIT;
    _lastInteractionTime = millis();
    _lastTempUpdate = 0;

    // Load settings from EEPROM
    _setpoint = SettingsManager::getInstance().getSetpoint();
    _fanSpeed = SettingsManager::getInstance().getFanSpeed();

    // Initial display update
    refreshDisplay();
}

void UIStateMachine::update() {
    auto& input = InputController::getInstance();

    // Handle auto-tune mode updates (no encoder input, just monitor progress)
    if (_mode == MODE_AUTOTUNE) {
        handleAutoTuneMode();
    }

    // Check for button press first
    if (input.wasButtonPressed()) {
        resetInactivityTimer();

        switch (_mode) {
            case MODE_NAVIGATE:
                if (_mainSelection == MAIN_SELECT_SETTINGS) {
                    _mode = MODE_SETTINGS;
                    _settingsSelection = SETTINGS_TEMP_UNIT;
                    DisplayManager::getInstance().showSettingsScreen();
                    input.playEnterBeep();
                } else {
                    // Show setpoint edit screen
                    _mode = MODE_SETPOINT;
                    DisplayManager::getInstance().showSetpointScreen(_setpoint);
                    input.playEnterBeep();
                }
                break;

            case MODE_SETPOINT:
                // Save setpoint and return to main screen
                SettingsManager::getInstance().setSetpoint(_setpoint);
                _mode = MODE_NAVIGATE;
                DisplayManager::getInstance().closeSetpointScreen();
                input.playExitBeep();
                break;

            case MODE_EDIT:
                _mode = MODE_NAVIGATE;
                SettingsManager::getInstance().setSetpoint(_setpoint);  // Save to EEPROM
                input.playExitBeep();
                break;

            case MODE_SETTINGS:
                handleSettingsButtonPress();
                break;

            case MODE_PID_MENU:
                handlePIDMenuButtonPress();
                break;

            case MODE_PID_EDIT:
                _mode = MODE_PID_MENU;
                input.playExitBeep();
                DisplayManager::getInstance().updatePIDScreen(_pidSelection, false, _pidSettingsChanged);
                break;

            case MODE_AUTOTUNE:
                handleAutoTuneButtonPress();
                break;

            case MODE_CURRENT:
                // Back button pressed - return to settings
                _mode = MODE_SETTINGS;
                _settingsSelection = SETTINGS_CURRENT;
                DisplayManager::getInstance().closeCurrentScreen();
                DisplayManager::getInstance().updateSettingsScreen(_settingsSelection);
                input.playExitBeep();
                break;

            case MODE_POWER:
                // Back button pressed - return to settings
                _mode = MODE_SETTINGS;
                _settingsSelection = SETTINGS_POWER;
                DisplayManager::getInstance().closePowerScreen();
                DisplayManager::getInstance().updateSettingsScreen(_settingsSelection);
                input.playExitBeep();
                break;

            case MODE_FAN:
                handleFanButtonPress();
                break;

            case MODE_FAN_SPEED:
                // Save fan speed and return to fan screen
                SettingsManager::getInstance().setFanSpeed(_fanSpeed);
                _mode = MODE_FAN;
                DisplayManager::getInstance().closeFanSpeedScreen();
                input.playExitBeep();
                break;

            case MODE_SMART_CONTROL:
                handleSmartControlButtonPress();
                break;

            case MODE_SMART_EDIT:
                // Exit edit mode, setpoint is already saved
                _mode = MODE_SMART_CONTROL;
                DisplayManager::getInstance().updateSmartControlScreen(
                    _smartSelection, false, SettingsManager::getInstance().getSmartControlEnabled());
                input.playExitBeep();
                break;

            case MODE_MAX_FAN_EDIT:
                // Exit edit mode, fan speed is already saved
                _mode = MODE_SMART_CONTROL;
                DisplayManager::getInstance().updateSmartControlScreen(
                    _smartSelection, false, SettingsManager::getInstance().getSmartControlEnabled());
                input.playExitBeep();
                break;

            case MODE_FIRMWARE:
                handleFirmwareButtonPress();
                break;

            case MODE_FIRMWARE_FLASHING:
                // Ignore button during flashing
                break;
        }

        refreshDisplay();
    }

    // Handle encoder input based on mode
    int delta = input.getEncoderDelta();
    if (delta != 0) {
        resetInactivityTimer();

        switch (_mode) {
            case MODE_NAVIGATE:
                handleNavigateMode(delta);
                break;
            case MODE_EDIT:
                handleEditMode(delta);
                break;
            case MODE_SETPOINT:
                handleSetpointMode(delta);
                break;
            case MODE_SETTINGS:
                handleSettingsMode(delta);
                break;
            case MODE_PID_MENU:
                handlePIDMenuMode(delta);
                break;
            case MODE_PID_EDIT:
                handlePIDEditMode(delta);
                break;
            case MODE_AUTOTUNE:
                // No encoder input during auto-tune
                break;
            case MODE_CURRENT:
                // No encoder input on current screen
                break;
            case MODE_POWER:
                // No encoder input on power screen
                break;
            case MODE_FAN:
                handleFanMode(delta);
                break;
            case MODE_FAN_SPEED:
                handleFanSpeedMode(delta);
                break;
            case MODE_SMART_CONTROL:
                handleSmartControlMode(delta);
                break;
            case MODE_SMART_EDIT:
                handleSmartEditMode(delta);
                break;
            case MODE_MAX_FAN_EDIT:
                handleMaxFanEditMode(delta);
                break;
            case MODE_FIRMWARE:
                handleFirmwareMode(delta);
                break;
            case MODE_FIRMWARE_FLASHING:
                // Ignore encoder during flashing
                break;
        }
    }

    // Update temperature reading
    updateTemperature();
}

void UIStateMachine::handleNavigateMode(int delta) {
    auto& input = InputController::getInstance();

    // Toggle between setpoint and settings
    if (_mainSelection == MAIN_SELECT_SETPOINT) {
        _mainSelection = MAIN_SELECT_SETTINGS;
    } else {
        _mainSelection = MAIN_SELECT_SETPOINT;
    }
    input.playNavigationBeep();
    refreshDisplay();
}

void UIStateMachine::handleEditMode(int delta) {
    // Adjust setpoint (delta is in detents, 0.1°C per detent)
    _setpoint += delta * 0.1f;

    // Clamp to valid range
    if (_setpoint < SETPOINT_MIN) _setpoint = SETPOINT_MIN;
    if (_setpoint > SETPOINT_MAX) _setpoint = SETPOINT_MAX;

    refreshDisplay();
}

void UIStateMachine::handleSetpointMode(int delta) {
    // Adjust setpoint (delta is in detents, 0.1°C per detent)
    _setpoint += delta * 0.1f;

    // Clamp to valid range
    if (_setpoint < SETPOINT_MIN) _setpoint = SETPOINT_MIN;
    if (_setpoint > SETPOINT_MAX) _setpoint = SETPOINT_MAX;

    // Update the setpoint screen
    DisplayManager::getInstance().updateSetpointScreen(_setpoint);
}

void UIStateMachine::handleSettingsMode(int delta) {
    auto& input = InputController::getInstance();

    // Navigate through settings items
    int newSelection = static_cast<int>(_settingsSelection) + delta;

    // Wrap around
    if (newSelection < 0) {
        newSelection = SETTINGS_ITEM_COUNT - 1;
    } else if (newSelection >= SETTINGS_ITEM_COUNT) {
        newSelection = 0;
    }

    _settingsSelection = static_cast<SettingsMenuItem>(newSelection);
    input.playNavigationBeep();
    DisplayManager::getInstance().updateSettingsScreen(_settingsSelection);
}

void UIStateMachine::handleSettingsButtonPress() {
    auto& input = InputController::getInstance();
    auto& settings = SettingsManager::getInstance();
    auto& display = DisplayManager::getInstance();

    switch (_settingsSelection) {
        case SETTINGS_TEMP_UNIT:
            settings.toggleTempUnit();
            input.playToggleBeep();
            display.updateSettingsScreen(_settingsSelection);
            break;

        case SETTINGS_PID:
            _mode = MODE_PID_MENU;
            _pidSelection = PID_MENU_MODE;
            _pidSettingsChanged = false;  // Reset dirty flag when entering PID menu
            DisplayManager::getInstance().showPIDScreen();
            input.playEnterBeep();
            break;

        case SETTINGS_CURRENT:
            _mode = MODE_CURRENT;
            display.showCurrentScreen();
            input.playEnterBeep();
            break;

        case SETTINGS_POWER:
            _mode = MODE_POWER;
            display.showPowerScreen();
            input.playEnterBeep();
            break;

        case SETTINGS_FANS:
            _mode = MODE_FAN;
            _fanSelection = FAN_SELECT_SPEED;
            display.showFanScreen();
            input.playEnterBeep();
            break;

        case SETTINGS_FIRMWARE:
            _mode = MODE_FIRMWARE;
            _firmwareSelection = FIRMWARE_UPDATE;  // Start on Update, skip version (display only)
            display.showFirmwareScreen();
            display.updateFirmwareScreen(_firmwareSelection, EXTIO2Flasher::getInstance().readVersion());
            input.playEnterBeep();
            break;

        case SETTINGS_BACK:
            _mode = MODE_NAVIGATE;
            _mainSelection = MAIN_SELECT_SETTINGS;
            display.closeSettingsScreen();
            input.playExitBeep();
            refreshDisplay();
            break;

        default:
            break;
    }
}

void UIStateMachine::handlePIDMenuMode(int delta) {
    auto& input = InputController::getInstance();

    int newSelection = static_cast<int>(_pidSelection) + delta;

    // Wrap around, skip AUTOTUNE always, skip SAVE if no changes
    auto shouldSkip = [this](int sel) {
        if (sel == PID_MENU_AUTOTUNE) return true;  // Auto-tune disabled
        if (sel == PID_MENU_SAVE && !_pidSettingsChanged) return true;
        return false;
    };

    do {
        if (newSelection < 0) {
            newSelection = PID_MENU_ITEM_COUNT - 1;
        } else if (newSelection >= PID_MENU_ITEM_COUNT) {
            newSelection = 0;
        }

        if (shouldSkip(newSelection)) {
            newSelection += (delta != 0) ? delta : 1;
            if (newSelection < 0) newSelection = PID_MENU_ITEM_COUNT - 1;
            else if (newSelection >= PID_MENU_ITEM_COUNT) newSelection = 0;
        }
    } while (shouldSkip(newSelection));

    _pidSelection = static_cast<PIDMenuItem>(newSelection);
    input.playNavigationBeep();
    DisplayManager::getInstance().updatePIDScreen(_pidSelection, false, _pidSettingsChanged);
}

void UIStateMachine::handlePIDMenuButtonPress() {
    auto& input = InputController::getInstance();
    auto& settings = SettingsManager::getInstance();
    auto& display = DisplayManager::getInstance();
    auto& pid = PIDController::getInstance();

    switch (_pidSelection) {
        case PID_MENU_MODE: {
            // Toggle between Off and On only
            PIDMode currentMode = settings.getPIDMode();
            PIDMode newMode = (currentMode == PID_OFF) ? PID_ON : PID_OFF;
            pid.setMode(newMode);
            _pidSettingsChanged = true;
            input.playToggleBeep();
            display.updatePIDScreen(_pidSelection, false, _pidSettingsChanged);
            break;
        }

        case PID_MENU_AUTOTUNE:
            // Start auto-tune and show auto-tune screen
            _mode = MODE_AUTOTUNE;
            pid.setMode(PID_AUTOTUNE);
            display.showAutoTuneScreen();
            input.playEnterBeep();
            break;

        case PID_MENU_KP:
        case PID_MENU_KI:
        case PID_MENU_KD:
        case PID_MENU_MIN:
        case PID_MENU_MAX:
            _mode = MODE_PID_EDIT;
            input.playEnterBeep();
            display.updatePIDScreen(_pidSelection, true, _pidSettingsChanged);
            break;

        case PID_MENU_SAVE:
            // Save all PID settings to EEPROM
            settings.save();
            _pidSettingsChanged = false;
            input.playEnterBeep();
            // Move selection to BACK (since SAVE is now hidden)
            _pidSelection = PID_MENU_BACK;
            display.updatePIDScreen(_pidSelection, false, _pidSettingsChanged);
            break;

        case PID_MENU_BACK:
            // Go back to settings without saving
            _mode = MODE_SETTINGS;
            _settingsSelection = SETTINGS_PID;
            display.closePIDScreen();
            display.updateSettingsScreen(_settingsSelection);
            input.playExitBeep();
            break;

        default:
            break;
    }
}

void UIStateMachine::handlePIDEditMode(int delta) {
    auto& settings = SettingsManager::getInstance();
    auto& pid = PIDController::getInstance();

    float kp = settings.getPIDKp();
    float ki = settings.getPIDKi();
    float kd = settings.getPIDKd();
    float minOut = settings.getPIDMinOutput();
    float maxOut = settings.getPIDMaxOutput();

    switch (_pidSelection) {
        case PID_MENU_KP:
            kp += delta * 0.1f;
            if (kp < 0.0f) kp = 0.0f;
            if (kp > 50.0f) kp = 50.0f;
            pid.setTunings(kp, ki, kd);
            _pidSettingsChanged = true;
            break;

        case PID_MENU_KI:
            ki += delta * 0.01f;
            if (ki < 0.0f) ki = 0.0f;
            if (ki > 10.0f) ki = 10.0f;
            pid.setTunings(kp, ki, kd);
            _pidSettingsChanged = true;
            break;

        case PID_MENU_KD:
            kd += delta * 0.1f;
            if (kd < 0.0f) kd = 0.0f;
            if (kd > 50.0f) kd = 50.0f;
            pid.setTunings(kp, ki, kd);
            _pidSettingsChanged = true;
            break;

        case PID_MENU_MIN:
            minOut += delta * 1.0f;
            if (minOut < 0.0f) minOut = 0.0f;
            if (minOut > maxOut) minOut = maxOut;
            pid.setOutputLimits(minOut, maxOut);
            _pidSettingsChanged = true;
            break;

        case PID_MENU_MAX:
            maxOut += delta * 1.0f;
            if (maxOut < minOut) maxOut = minOut;
            if (maxOut > 100.0f) maxOut = 100.0f;
            pid.setOutputLimits(minOut, maxOut);
            _pidSettingsChanged = true;
            break;

        default:
            break;
    }

    DisplayManager::getInstance().updatePIDScreen(_pidSelection, true, _pidSettingsChanged);
}

void UIStateMachine::handleAutoTuneMode() {
    auto& pid = PIDController::getInstance();
    auto& display = DisplayManager::getInstance();

    // Update auto-tune screen with current progress
    if (pid.isAutoTuning()) {
        int cycle = pid.getAutoTuneCycle();
        const char* status = pid.isAutoTuneCooling() ? "Cooling..." : "Heating...";
        display.updateAutoTuneScreen(cycle, 5, status);
    }

    // Check if auto-tune completed
    if (pid.checkAndClearAutoTuneComplete()) {
        extern void logPrintf(const char* format, ...);
        logPrintf("Auto-tune complete handler: setting mode to PID_MENU\n");
        // Auto-tune finished successfully - go back to PID menu
        _mode = MODE_PID_MENU;
        _pidSettingsChanged = true;  // Mark as changed since we have new values
        logPrintf("Auto-tune complete handler: closing auto-tune screen\n");
        display.closeAutoTuneScreen();  // This creates and loads PID screen
        logPrintf("Auto-tune complete handler: updating PID screen\n");
        display.updatePIDScreen(_pidSelection, false, _pidSettingsChanged);
        logPrintf("Auto-tune complete handler: done, mode=%d\n", _mode);
        InputController::getInstance().playEnterBeep();
        return;  // Don't run other checks this frame
    }

    // Check if auto-tune was stopped/failed
    if (!pid.isAutoTuning() && _mode == MODE_AUTOTUNE) {
        // Auto-tune stopped (timeout or error)
        display.showAutoTuneError("Timeout/Failed");
    }
}

void UIStateMachine::handleAutoTuneButtonPress() {
    auto& pid = PIDController::getInstance();
    auto& display = DisplayManager::getInstance();
    auto& input = InputController::getInstance();

    // Cancel button pressed - stop auto-tune and go back to PID menu
    if (pid.isAutoTuning()) {
        pid.stopAutoTune();
    }

    _mode = MODE_PID_MENU;
    display.closeAutoTuneScreen();
    display.updatePIDScreen(_pidSelection, false, _pidSettingsChanged);
    input.playExitBeep();
}

void UIStateMachine::handleFanMode(int delta) {
    auto& input = InputController::getInstance();

    // Cycle through speed -> back -> smart -> speed
    int newSelection = static_cast<int>(_fanSelection) + delta;
    if (newSelection < 0) newSelection = FAN_SELECT_SMART;
    else if (newSelection > FAN_SELECT_SMART) newSelection = FAN_SELECT_SPEED;
    _fanSelection = static_cast<FanScreenSelection>(newSelection);

    input.playNavigationBeep();
    DisplayManager::getInstance().updateFanScreen(FanController::getInstance().getAverageRPM(), _fanSpeed, _fanSelection);
}

void UIStateMachine::handleFanButtonPress() {
    auto& display = DisplayManager::getInstance();
    auto& input = InputController::getInstance();

    switch (_fanSelection) {
        case FAN_SELECT_SPEED:
            // Enter fan speed edit mode
            _mode = MODE_FAN_SPEED;
            display.showFanSpeedScreen(_fanSpeed);
            input.playEnterBeep();
            break;

        case FAN_SELECT_BACK:
            // Return to settings
            _mode = MODE_SETTINGS;
            _settingsSelection = SETTINGS_FANS;
            display.closeFanScreen();
            display.updateSettingsScreen(_settingsSelection);
            input.playExitBeep();
            break;

        case FAN_SELECT_SMART:
            // Enter smart control menu
            _mode = MODE_SMART_CONTROL;
            _smartSelection = SMART_CONTROL_TOGGLE;
            display.showSmartControlScreen();
            input.playEnterBeep();
            break;
    }
}

void UIStateMachine::handleFanSpeedMode(int delta) {
    // Adjust fan speed (delta is in detents, 1% per detent)
    _fanSpeed += delta * 1.0f;

    // Clamp to valid range
    if (_fanSpeed < 0.0f) _fanSpeed = 0.0f;
    if (_fanSpeed > 100.0f) _fanSpeed = 100.0f;

    DisplayManager::getInstance().updateFanSpeedScreen(_fanSpeed);
}

void UIStateMachine::handleSmartControlMode(int delta) {
    auto& input = InputController::getInstance();
    auto& settings = SettingsManager::getInstance();

    int newSelection = static_cast<int>(_smartSelection) + delta;

    // Wrap around, but skip setpoint if smart control is off
    do {
        if (newSelection < 0) newSelection = SMART_CONTROL_ITEM_COUNT - 1;
        else if (newSelection >= SMART_CONTROL_ITEM_COUNT) newSelection = 0;

        // Skip setpoint if smart control is disabled
        if (newSelection == SMART_CONTROL_SETPOINT && !settings.getSmartControlEnabled()) {
            newSelection += delta;
            if (newSelection < 0) newSelection = SMART_CONTROL_ITEM_COUNT - 1;
            else if (newSelection >= SMART_CONTROL_ITEM_COUNT) newSelection = 0;
        }
    } while (newSelection == SMART_CONTROL_SETPOINT && !settings.getSmartControlEnabled());

    _smartSelection = static_cast<SmartControlMenuItem>(newSelection);
    input.playNavigationBeep();
    DisplayManager::getInstance().updateSmartControlScreen(_smartSelection, false, settings.getSmartControlEnabled());
}

void UIStateMachine::handleSmartControlButtonPress() {
    auto& display = DisplayManager::getInstance();
    auto& input = InputController::getInstance();
    auto& settings = SettingsManager::getInstance();

    switch (_smartSelection) {
        case SMART_CONTROL_TOGGLE:
            // Toggle smart control on/off
            settings.setSmartControlEnabled(!settings.getSmartControlEnabled());
            input.playToggleBeep();
            display.updateSmartControlScreen(_smartSelection, false, settings.getSmartControlEnabled());
            break;

        case SMART_CONTROL_SETPOINT:
            // Only allow editing if smart control is enabled
            if (settings.getSmartControlEnabled()) {
                _mode = MODE_SMART_EDIT;
                input.playEnterBeep();
                display.updateSmartControlScreen(_smartSelection, true, settings.getSmartControlEnabled());
            }
            break;

        case SMART_CONTROL_MAX_FAN:
            // Always allow editing max fan speed
            _mode = MODE_MAX_FAN_EDIT;
            input.playEnterBeep();
            display.updateSmartControlScreen(_smartSelection, true, settings.getSmartControlEnabled());
            break;

        case SMART_CONTROL_BACK:
            // Return to fan screen
            _mode = MODE_FAN;
            _fanSelection = FAN_SELECT_SMART;
            display.closeSmartControlScreen();
            display.updateFanScreen(FanController::getInstance().getAverageRPM(), _fanSpeed, _fanSelection);
            input.playExitBeep();
            break;

        default:
            break;
    }
}

void UIStateMachine::handleSmartEditMode(int delta) {
    auto& settings = SettingsManager::getInstance();

    float setpoint = settings.getSmartSetpoint();
    setpoint += delta * 1.0f;

    // Clamp to valid range
    if (setpoint < 0.0f) setpoint = 0.0f;
    if (setpoint > 100.0f) setpoint = 100.0f;

    settings.setSmartSetpoint(setpoint);
    DisplayManager::getInstance().updateSmartControlScreen(_smartSelection, true, settings.getSmartControlEnabled());
}

void UIStateMachine::handleMaxFanEditMode(int delta) {
    auto& settings = SettingsManager::getInstance();

    float fanSpeed = settings.getFanSpeed();
    fanSpeed += delta * 1.0f;

    // Clamp to valid range
    if (fanSpeed < 0.0f) fanSpeed = 0.0f;
    if (fanSpeed > 100.0f) fanSpeed = 100.0f;

    settings.setFanSpeed(fanSpeed);
    _fanSpeed = fanSpeed;

    // Apply to fan controller immediately
    FanController::getInstance().setSpeed(static_cast<uint8_t>(fanSpeed));

    DisplayManager::getInstance().updateSmartControlScreen(_smartSelection, true, settings.getSmartControlEnabled());
}

void UIStateMachine::handleFirmwareMode(int delta) {
    auto& input = InputController::getInstance();
    auto& display = DisplayManager::getInstance();

    int newSelection = static_cast<int>(_firmwareSelection) + delta;

    // Wrap around, skip VERSION (display only)
    if (newSelection < FIRMWARE_UPDATE) newSelection = FIRMWARE_BACK;
    else if (newSelection > FIRMWARE_BACK) newSelection = FIRMWARE_UPDATE;

    _firmwareSelection = static_cast<FirmwareMenuItem>(newSelection);
    input.playNavigationBeep();
    display.updateFirmwareScreen(_firmwareSelection, EXTIO2Flasher::getInstance().readVersion());
}

void UIStateMachine::handleFirmwareButtonPress() {
    auto& display = DisplayManager::getInstance();
    auto& input = InputController::getInstance();
    auto& flasher = EXTIO2Flasher::getInstance();

    switch (_firmwareSelection) {
        case FIRMWARE_VERSION:
            // Display only - do nothing
            break;

        case FIRMWARE_UPDATE:
            // Flash custom firmware
            _mode = MODE_FIRMWARE_FLASHING;
            display.showFlashingProgress(0, 11, "Flashing custom...");
            input.playEnterBeep();

            if (flasher.flashFirmware(extio2_custom_firmware, sizeof(extio2_custom_firmware),
                [&display](int current, int total) {
                    display.showFlashingProgress(current, total, "Flashing custom...");
                    display.update();
                })) {
                // Success
                display.showFlashingProgress(11, 11, "Success!");
                display.update();
                delay(1000);
            } else {
                // Failed
                display.showFlashingProgress(0, 11, flasher.getLastError());
                display.update();
                delay(2000);
            }

            // Return to firmware menu
            _mode = MODE_FIRMWARE;
            display.updateFirmwareScreen(_firmwareSelection, flasher.readVersion());
            break;

        case FIRMWARE_RESTORE:
            // Flash original firmware
            _mode = MODE_FIRMWARE_FLASHING;
            display.showFlashingProgress(0, 11, "Restoring...");
            input.playEnterBeep();

            if (flasher.flashFirmware(extio2_original_firmware, sizeof(extio2_original_firmware),
                [&display](int current, int total) {
                    display.showFlashingProgress(current, total, "Restoring...");
                    display.update();
                })) {
                // Success
                display.showFlashingProgress(11, 11, "Restored!");
                display.update();
                delay(1000);
            } else {
                // Failed
                display.showFlashingProgress(0, 11, flasher.getLastError());
                display.update();
                delay(2000);
            }

            // Return to firmware menu
            _mode = MODE_FIRMWARE;
            display.updateFirmwareScreen(_firmwareSelection, flasher.readVersion());
            break;

        case FIRMWARE_BACK:
            // Return to settings
            _mode = MODE_SETTINGS;
            _settingsSelection = SETTINGS_FIRMWARE;
            display.closeFirmwareScreen();
            display.updateSettingsScreen(_settingsSelection);
            input.playExitBeep();
            break;

        default:
            break;
    }
}

void UIStateMachine::updateTemperature() {
    unsigned long now = millis();

    if (now - _lastTempUpdate >= TEMP_UPDATE_INTERVAL) {
        float temp = TemperatureSensor::getInstance().readTemperature();
        _sensorError = isnan(temp);

        if (!_sensorError) {
            _currentTemp = temp;
        }

        _lastTempUpdate = now;

        // Only refresh display if not in settings
        if (_mode != MODE_SETTINGS) {
            refreshDisplay();
        }
    }
}

void UIStateMachine::refreshDisplay() {
    auto& display = DisplayManager::getInstance();
    extern void logPrintf(const char* format, ...);

    switch (_mode) {
        case MODE_SETTINGS:
            display.updateSettingsScreen(_settingsSelection);
            break;

        case MODE_PID_MENU:
            display.updatePIDScreen(_pidSelection, false, _pidSettingsChanged);
            break;

        case MODE_PID_EDIT:
            display.updatePIDScreen(_pidSelection, true, _pidSettingsChanged);
            break;

        case MODE_AUTOTUNE:
            // Auto-tune screen updates itself in handleAutoTuneMode()
            break;

        case MODE_CURRENT:
            // Current screen updates from main loop
            break;

        case MODE_SETPOINT:
            display.updateSetpointScreen(_setpoint);
            break;

        case MODE_POWER:
            // Power screen updates from main loop
            break;

        case MODE_FAN:
            display.updateFanScreen(FanController::getInstance().getAverageRPM(), _fanSpeed, _fanSelection);
            break;

        case MODE_FAN_SPEED:
            display.updateFanSpeedScreen(_fanSpeed);
            break;

        case MODE_SMART_CONTROL:
            display.updateSmartControlScreen(_smartSelection, false,
                SettingsManager::getInstance().getSmartControlEnabled());
            break;

        case MODE_SMART_EDIT:
            display.updateSmartControlScreen(_smartSelection, true,
                SettingsManager::getInstance().getSmartControlEnabled());
            break;

        case MODE_MAX_FAN_EDIT:
            display.updateSmartControlScreen(_smartSelection, true,
                SettingsManager::getInstance().getSmartControlEnabled());
            break;

        case MODE_FIRMWARE:
            display.updateFirmwareScreen(_firmwareSelection, EXTIO2Flasher::getInstance().readVersion());
            break;

        case MODE_FIRMWARE_FLASHING:
            // Progress screen updates during flash operation
            break;

        case MODE_NAVIGATE:
        case MODE_EDIT:
        default: {
            bool editing = (_mode == MODE_EDIT);
            display.updateMainScreen(_currentTemp, _setpoint, _mainSelection, editing, _sensorError);
            break;
        }
    }
}

float UIStateMachine::getSetpoint() const {
    return _setpoint;
}

float UIStateMachine::getCurrentTemp() const {
    return _currentTemp;
}

void UIStateMachine::resetInactivityTimer() {
    _lastInteractionTime = millis();
}

bool UIStateMachine::isInactive() const {
    return (millis() - _lastInteractionTime) > INACTIVITY_DELAY;
}

unsigned long UIStateMachine::getInactivityTime() const {
    return millis() - _lastInteractionTime;
}
