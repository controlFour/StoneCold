#ifndef UI_STATE_MACHINE_H
#define UI_STATE_MACHINE_H

#include "DisplayManager.h"

enum UIMode {
    MODE_NAVIGATE,
    MODE_EDIT,
    MODE_SETTINGS,
    MODE_PID_MENU,
    MODE_PID_EDIT,
    MODE_AUTOTUNE,
    MODE_CURRENT,
    MODE_SETPOINT,
    MODE_POWER,
    MODE_FAN,
    MODE_FAN_SPEED,
    MODE_SMART_CONTROL,
    MODE_SMART_EDIT,
    MODE_MAX_FAN_EDIT,
    MODE_FIRMWARE,
    MODE_FIRMWARE_FLASHING
};

class UIStateMachine {
public:
    static UIStateMachine& getInstance();

    void begin();
    void update();

    float getSetpoint() const;
    float getCurrentTemp() const;
    bool hasSensorError() const { return _sensorError; }

    void resetInactivityTimer();
    bool isInactive() const;
    unsigned long getInactivityTime() const;

private:
    UIStateMachine() = default;
    UIStateMachine(const UIStateMachine&) = delete;
    UIStateMachine& operator=(const UIStateMachine&) = delete;

    void handleNavigateMode(int delta);
    void handleEditMode(int delta);
    void handleSetpointMode(int delta);
    void handleSettingsMode(int delta);
    void handleSettingsButtonPress();
    void handlePIDMenuMode(int delta);
    void handlePIDMenuButtonPress();
    void handlePIDEditMode(int delta);
    void handleAutoTuneMode();
    void handleAutoTuneButtonPress();
    void handleFanMode(int delta);
    void handleFanButtonPress();
    void handleFanSpeedMode(int delta);
    void handleSmartControlMode(int delta);
    void handleSmartControlButtonPress();
    void handleSmartEditMode(int delta);
    void handleMaxFanEditMode(int delta);
    void handleFirmwareMode(int delta);
    void handleFirmwareButtonPress();
    void updateTemperature();
    void refreshDisplay();

    UIMode _mode = MODE_NAVIGATE;
    MainScreenSelection _mainSelection = MAIN_SELECT_SETPOINT;
    SettingsMenuItem _settingsSelection = SETTINGS_TEMP_UNIT;
    PIDMenuItem _pidSelection = PID_MENU_MODE;
    bool _pidSettingsChanged = false;  // Track if PID settings need saving
    FanScreenSelection _fanSelection = FAN_SELECT_SPEED;
    float _fanSpeed = 100.0f;  // Fan speed setpoint (0-100%)
    SmartControlMenuItem _smartSelection = SMART_CONTROL_TOGGLE;
    FirmwareMenuItem _firmwareSelection = FIRMWARE_VERSION;

    float _setpoint = 22.0f;
    float _currentTemp = 25.0f;
    bool _sensorError = false;

    unsigned long _lastInteractionTime = 0;
    unsigned long _lastTempUpdate = 0;

    static constexpr unsigned long INACTIVITY_DELAY = 3000;  // 3 seconds
    static constexpr unsigned long TEMP_UPDATE_INTERVAL = 1000;  // 1 second
    static constexpr float SETPOINT_MIN = -12.2f;  // 10°F
    static constexpr float SETPOINT_MAX = 35.0f;
    static constexpr float SETPOINT_STEP = 0.25f;  // Per encoder pulse
};

#endif
