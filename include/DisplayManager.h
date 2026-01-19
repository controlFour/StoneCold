#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <lvgl.h>
#include "SettingsManager.h"

// Selection states for main screen
enum MainScreenSelection {
    MAIN_SELECT_SETPOINT,
    MAIN_SELECT_SETTINGS
};

// Settings menu items
enum SettingsMenuItem {
    SETTINGS_TEMP_UNIT,
    SETTINGS_PID,
    SETTINGS_CURRENT,
    SETTINGS_POWER,
    SETTINGS_FANS,
    SETTINGS_FIRMWARE,
    SETTINGS_BACK,
    SETTINGS_ITEM_COUNT
};

// Firmware menu items
enum FirmwareMenuItem {
    FIRMWARE_VERSION,     // Shows "Version: X" (display only)
    FIRMWARE_UPDATE,      // "Update GPIO FW"
    FIRMWARE_RESTORE,     // "Restore Original"
    FIRMWARE_BACK,
    FIRMWARE_ITEM_COUNT
};

// Fan screen selection
enum FanScreenSelection {
    FAN_SELECT_SPEED,
    FAN_SELECT_BACK,
    FAN_SELECT_SMART
};

// Smart control menu items
enum SmartControlMenuItem {
    SMART_CONTROL_TOGGLE,
    SMART_CONTROL_SETPOINT,
    SMART_CONTROL_MAX_FAN,
    SMART_CONTROL_BACK,
    SMART_CONTROL_ITEM_COUNT
};

// PID menu items
enum PIDMenuItem {
    PID_MENU_MODE,
    PID_MENU_AUTOTUNE,
    PID_MENU_KP,
    PID_MENU_KI,
    PID_MENU_KD,
    PID_MENU_MIN,
    PID_MENU_MAX,
    PID_MENU_SAVE,    // Only shown when hasChanges
    PID_MENU_BACK,
    PID_MENU_ITEM_COUNT
};

class DisplayManager {
public:
    static DisplayManager& getInstance();

    void begin();
    void update();  // Calls lv_timer_handler()

    // Main screen
    void showMainScreen();
    void updateMainScreen(float tempCelsius, float setpointCelsius,
                          MainScreenSelection selection, bool editing,
                          bool sensorError = false);

    // Settings screen
    void showSettingsScreen();
    void updateSettingsScreen(SettingsMenuItem selectedItem);
    void closeSettingsScreen();
    bool isSettingsScreenVisible() const;

    // PID screen
    void showPIDScreen();
    void updatePIDScreen(PIDMenuItem selectedItem, bool editing, bool hasChanges);
    void closePIDScreen();
    bool isPIDScreenVisible() const;

    // Auto-tune screen
    void showAutoTuneScreen();
    void updateAutoTuneScreen(int cycle, int totalCycles, const char* status);
    void showAutoTuneError(const char* error);
    void closeAutoTuneScreen();
    bool isAutoTuneScreenVisible() const;

    // Current monitor screen
    void showCurrentScreen();
    void updateCurrentScreen(float amps);
    void closeCurrentScreen();
    bool isCurrentScreenVisible() const;

    // Setpoint edit screen
    void showSetpointScreen(float setpointCelsius);
    void updateSetpointScreen(float setpointCelsius);
    void closeSetpointScreen();
    bool isSetpointScreenVisible() const;

    // Power monitor screen
    void showPowerScreen();
    void updatePowerScreen(float powerPercent);
    void closePowerScreen();
    bool isPowerScreenVisible() const;

    // Fan screen
    void showFanScreen();
    void updateFanScreen(int rpm, float speedPercent, FanScreenSelection selection);
    void closeFanScreen();
    bool isFanScreenVisible() const;

    // Fan speed edit screen
    void showFanSpeedScreen(float speedPercent);
    void updateFanSpeedScreen(float speedPercent);
    void closeFanSpeedScreen();
    bool isFanSpeedScreenVisible() const;

    // Smart control screen
    void showSmartControlScreen();
    void updateSmartControlScreen(SmartControlMenuItem selectedItem, bool editing, bool smartEnabled);
    void closeSmartControlScreen();
    bool isSmartControlScreenVisible() const;

    // Firmware screen
    void showFirmwareScreen();
    void updateFirmwareScreen(FirmwareMenuItem selectedItem, uint8_t version);
    void showFlashingProgress(int currentPage, int totalPages, const char* status);
    void closeFirmwareScreen();
    bool isFirmwareScreenVisible() const;

    // Get active screen for snow effect
    lv_obj_t* getActiveScreen();

private:
    DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;

    void initLVGL();
    void createMainScreen();
    void createSettingsScreen();
    void createPIDScreen();
    void createAutoTuneScreen();
    void createCurrentScreen();
    void createSetpointScreen();
    void createPowerScreen();
    void createFanScreen();
    void createFanSpeedScreen();
    void createSmartControlScreen();
    void createFirmwareScreen();

    static void dispFlushCallback(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
    static void inputReadCallback(lv_indev_drv_t* indev_driver, lv_indev_data_t* data);

    // Display configuration
    static constexpr uint16_t SCREEN_WIDTH = 240;
    static constexpr uint16_t SCREEN_HEIGHT = 240;
    static constexpr size_t DRAW_BUF_SIZE = SCREEN_WIDTH * 10;

    // LVGL buffers and drivers
    lv_disp_draw_buf_t _drawBuf;
    lv_color_t _drawBufData[DRAW_BUF_SIZE];

    // Screens
    lv_obj_t* _mainScreen = nullptr;
    lv_obj_t* _settingsScreen = nullptr;

    // Main screen elements
    lv_obj_t* _tempLabel = nullptr;
    lv_obj_t* _setpointLabel = nullptr;
    lv_obj_t* _settingsIcon = nullptr;

    // Settings screen elements
    lv_obj_t* _settingsTitle = nullptr;
    lv_obj_t* _settingsItems[SETTINGS_ITEM_COUNT] = {nullptr};

    // PID screen
    lv_obj_t* _pidScreen = nullptr;
    lv_obj_t* _pidTitle = nullptr;
    lv_obj_t* _pidContainer = nullptr;
    lv_obj_t* _pidItems[PID_MENU_ITEM_COUNT] = {nullptr};

    // Auto-tune screen
    lv_obj_t* _autoTuneScreen = nullptr;
    lv_obj_t* _autoTuneTitle = nullptr;
    lv_obj_t* _autoTuneStatus = nullptr;
    lv_obj_t* _autoTuneProgress = nullptr;
    lv_obj_t* _autoTuneCancel = nullptr;

    // Current monitor screen
    lv_obj_t* _currentScreen = nullptr;
    lv_obj_t* _currentTitle = nullptr;
    lv_obj_t* _currentValue = nullptr;
    lv_obj_t* _currentBack = nullptr;

    // Setpoint edit screen
    lv_obj_t* _setpointScreen = nullptr;
    lv_obj_t* _setpointTitle = nullptr;
    lv_obj_t* _setpointValue = nullptr;

    // Power monitor screen
    lv_obj_t* _powerScreen = nullptr;
    lv_obj_t* _powerTitle = nullptr;
    lv_obj_t* _powerValue = nullptr;
    lv_obj_t* _powerBack = nullptr;

    // Fan screen
    lv_obj_t* _fanScreen = nullptr;
    lv_obj_t* _fanSpeedLabel = nullptr;   // Top - fan speed setpoint
    lv_obj_t* _fanRPMLabel = nullptr;     // Center - large RPM display
    lv_obj_t* _fanBackLabel = nullptr;    // Bottom left - back button
    lv_obj_t* _fanSmartLabel = nullptr;   // Bottom right - smart control

    // Fan speed edit screen
    lv_obj_t* _fanSpeedScreen = nullptr;
    lv_obj_t* _fanSpeedTitle = nullptr;
    lv_obj_t* _fanSpeedValue = nullptr;

    // Smart control screen
    lv_obj_t* _smartControlScreen = nullptr;
    lv_obj_t* _smartControlTitle = nullptr;
    lv_obj_t* _smartControlItems[SMART_CONTROL_ITEM_COUNT] = {nullptr};

    // Firmware screen
    lv_obj_t* _firmwareScreen = nullptr;
    lv_obj_t* _firmwareTitle = nullptr;
    lv_obj_t* _firmwareItems[FIRMWARE_ITEM_COUNT] = {nullptr};
    lv_obj_t* _firmwareProgress = nullptr;
    lv_obj_t* _firmwareStatus = nullptr;

    bool _settingsVisible = false;
    bool _pidVisible = false;
    bool _autoTuneVisible = false;
    bool _currentVisible = false;
    bool _setpointVisible = false;
    bool _powerVisible = false;
    bool _fanVisible = false;
    bool _fanSpeedVisible = false;
    bool _smartControlVisible = false;
    bool _firmwareVisible = false;
};

#endif
