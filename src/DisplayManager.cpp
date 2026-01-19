#include "DisplayManager.h"
#include "FanController.h"
#include <M5Dial.h>
#include "settings_img.h"

// Declare custom fonts
LV_FONT_DECLARE(lv_font_montserrat_60);
LV_FONT_DECLARE(lv_font_montserrat_72);
LV_FONT_DECLARE(lv_font_montserrat_84);
LV_FONT_DECLARE(lv_font_montserrat_96);

DisplayManager& DisplayManager::getInstance() {
    static DisplayManager instance;
    return instance;
}

void DisplayManager::begin() {
    // Initialize display hardware
    M5.Display.setBrightness(200);
    M5.Display.setRotation(0);
    M5.Display.fillScreen(TFT_BLACK);

    // Initialize LVGL
    initLVGL();

    // Create main screen
    createMainScreen();
}

void DisplayManager::update() {
    lv_timer_handler();
}

void DisplayManager::initLVGL() {
    lv_init();

    // Initialize the display buffer
    lv_disp_draw_buf_init(&_drawBuf, _drawBufData, nullptr, DRAW_BUF_SIZE);

    // Initialize the display driver
    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = SCREEN_WIDTH;
    dispDrv.ver_res = SCREEN_HEIGHT;
    dispDrv.flush_cb = dispFlushCallback;
    dispDrv.draw_buf = &_drawBuf;
    lv_disp_drv_register(&dispDrv);

    // Initialize the input device driver
    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type = LV_INDEV_TYPE_ENCODER;
    indevDrv.read_cb = inputReadCallback;
    lv_indev_drv_register(&indevDrv);
}

void DisplayManager::dispFlushCallback(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    M5.Display.startWrite();
    M5.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5.Display.pushPixels(reinterpret_cast<uint16_t*>(&color_p->full), w * h, true);
    M5.Display.endWrite();

    lv_disp_flush_ready(disp);
}

void DisplayManager::inputReadCallback(lv_indev_drv_t* indev_driver, lv_indev_data_t* data) {
    // We handle input manually, not through LVGL
    data->state = LV_INDEV_STATE_RELEASED;
    data->enc_diff = 0;
}

void DisplayManager::createMainScreen() {
    _mainScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_mainScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_mainScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_mainScreen, LV_SCROLLBAR_MODE_OFF);

    // Setpoint label (small, at top)
    _setpointLabel = lv_label_create(_mainScreen);
    lv_obj_align(_setpointLabel, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_setpointLabel, lv_color_hex(0xffff00), 0);
    lv_obj_set_style_text_font(_setpointLabel, &lv_font_montserrat_20, 0);

    // Temperature label (large, center)
    _tempLabel = lv_label_create(_mainScreen);
    lv_obj_align(_tempLabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_tempLabel, &lv_font_montserrat_96, 0);

    // Settings icon (PNG image)
    _settingsIcon = lv_img_create(_mainScreen);
    lv_img_set_src(_settingsIcon, &settings_img);
    lv_obj_align(_settingsIcon, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_img_recolor_opa(_settingsIcon, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor(_settingsIcon, lv_color_hex(0x888888), 0);

    // Load the main screen
    lv_scr_load(_mainScreen);
}

void DisplayManager::showMainScreen() {
    extern void logPrintf(const char* format, ...);
    logPrintf("showMainScreen: called!\n");
    if (!_mainScreen) {
        createMainScreen();
    }
    lv_scr_load(_mainScreen);
    _settingsVisible = false;
}

void DisplayManager::updateMainScreen(float tempCelsius, float setpointCelsius,
                                       MainScreenSelection selection, bool editing,
                                       bool sensorError) {
    if (!_mainScreen) return;

    auto& settings = SettingsManager::getInstance();
    float displaySetpoint = settings.toDisplayUnit(setpointCelsius);
    const char* unitStr = (settings.getTempUnit() == CELSIUS) ? "C" : "F";

    // Update temperature label (show "Error" if sensor failed)
    if (sensorError) {
        lv_label_set_text(_tempLabel, "Error");
        lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0xff0000), 0);  // Red
    } else {
        float displayTemp = settings.toDisplayUnit(tempCelsius);
        lv_label_set_text_fmt(_tempLabel, "%.1f%s", displayTemp, "°");
        lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0xffffff), 0);  // White
    }

    // Update setpoint label
    lv_label_set_text_fmt(_setpointLabel, "%.1f°%s", displaySetpoint, unitStr);

    // Update setpoint color based on selection state
    if (selection == MAIN_SELECT_SETPOINT) {
        if (editing) {
            lv_obj_set_style_text_color(_setpointLabel, lv_color_hex(0x00ff00), 0);  // Green when editing
        } else {
            lv_obj_set_style_text_color(_setpointLabel, lv_color_hex(0xffff00), 0);  // Yellow when selected
        }
    } else {
        lv_obj_set_style_text_color(_setpointLabel, lv_color_hex(0x00aaff), 0);  // Cyan when not selected
    }

    // Update settings icon color
    if (selection == MAIN_SELECT_SETTINGS) {
        lv_obj_set_style_img_recolor(_settingsIcon, lv_color_hex(0xffff00), 0);  // Yellow when selected
    } else {
        lv_obj_set_style_img_recolor(_settingsIcon, lv_color_hex(0x888888), 0);  // Gray when not selected
    }

    // Force refresh
    lv_obj_invalidate(_tempLabel);
    lv_obj_invalidate(_setpointLabel);
    lv_obj_invalidate(_settingsIcon);
    lv_refr_now(nullptr);
}

void DisplayManager::createSettingsScreen() {
    _settingsScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_settingsScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_settingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_settingsScreen, LV_SCROLLBAR_MODE_OFF);

    // Title
    _settingsTitle = lv_label_create(_settingsScreen);
    lv_label_set_text(_settingsTitle, "Settings");
    lv_obj_align(_settingsTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_settingsTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_settingsTitle, &lv_font_montserrat_20, 0);

    // Temperature unit item
    _settingsItems[SETTINGS_TEMP_UNIT] = lv_label_create(_settingsScreen);
    lv_obj_align(_settingsItems[SETTINGS_TEMP_UNIT], LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_text_font(_settingsItems[SETTINGS_TEMP_UNIT], &lv_font_montserrat_20, 0);

    // PID item
    _settingsItems[SETTINGS_PID] = lv_label_create(_settingsScreen);
    lv_label_set_text(_settingsItems[SETTINGS_PID], "PID");
    lv_obj_align(_settingsItems[SETTINGS_PID], LV_ALIGN_CENTER, 0, -36);
    lv_obj_set_style_text_font(_settingsItems[SETTINGS_PID], &lv_font_montserrat_20, 0);

    // Current item
    _settingsItems[SETTINGS_CURRENT] = lv_label_create(_settingsScreen);
    lv_label_set_text(_settingsItems[SETTINGS_CURRENT], "Current (A)");
    lv_obj_align(_settingsItems[SETTINGS_CURRENT], LV_ALIGN_CENTER, 0, -12);
    lv_obj_set_style_text_font(_settingsItems[SETTINGS_CURRENT], &lv_font_montserrat_20, 0);

    // Power item
    _settingsItems[SETTINGS_POWER] = lv_label_create(_settingsScreen);
    lv_label_set_text(_settingsItems[SETTINGS_POWER], "Power (%)");
    lv_obj_align(_settingsItems[SETTINGS_POWER], LV_ALIGN_CENTER, 0, 12);
    lv_obj_set_style_text_font(_settingsItems[SETTINGS_POWER], &lv_font_montserrat_20, 0);

    // Fans item
    _settingsItems[SETTINGS_FANS] = lv_label_create(_settingsScreen);
    lv_label_set_text(_settingsItems[SETTINGS_FANS], "Fans");
    lv_obj_align(_settingsItems[SETTINGS_FANS], LV_ALIGN_CENTER, 0, 36);
    lv_obj_set_style_text_font(_settingsItems[SETTINGS_FANS], &lv_font_montserrat_20, 0);

    // Firmware item
    _settingsItems[SETTINGS_FIRMWARE] = lv_label_create(_settingsScreen);
    lv_label_set_text(_settingsItems[SETTINGS_FIRMWARE], "Firmware");
    lv_obj_align(_settingsItems[SETTINGS_FIRMWARE], LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_text_font(_settingsItems[SETTINGS_FIRMWARE], &lv_font_montserrat_20, 0);

    // Back item
    _settingsItems[SETTINGS_BACK] = lv_label_create(_settingsScreen);
    lv_label_set_text(_settingsItems[SETTINGS_BACK], "< Back");
    lv_obj_align(_settingsItems[SETTINGS_BACK], LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(_settingsItems[SETTINGS_BACK], &lv_font_montserrat_20, 0);
}

void DisplayManager::showSettingsScreen() {
    if (!_settingsScreen) {
        createSettingsScreen();
    }
    lv_scr_load(_settingsScreen);
    _settingsVisible = true;
    updateSettingsScreen(SETTINGS_TEMP_UNIT);
}

void DisplayManager::updateSettingsScreen(SettingsMenuItem selectedItem) {
    if (!_settingsScreen) return;

    auto& settings = SettingsManager::getInstance();
    const char* unitStr = (settings.getTempUnit() == CELSIUS) ? "°C" : "°F";
    lv_label_set_text(_settingsItems[SETTINGS_TEMP_UNIT], unitStr);

    // Update colors for all items
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        if (i == selectedItem) {
            lv_obj_set_style_text_color(_settingsItems[i], lv_color_hex(0xffff00), 0);  // Yellow
        } else {
            lv_obj_set_style_text_color(_settingsItems[i], lv_color_hex(0x888888), 0);  // Gray
        }
    }

    lv_refr_now(nullptr);
}

void DisplayManager::closeSettingsScreen() {
    // Clear old main screen pointer (may have been invalidated by LVGL)
    _mainScreen = nullptr;
    _tempLabel = nullptr;
    _setpointLabel = nullptr;
    _settingsIcon = nullptr;

    // Create and load main screen FIRST (before deleting current screen)
    createMainScreen();
    _settingsVisible = false;

    // Now safe to delete settings screen (no longer active)
    if (_settingsScreen) {
        lv_obj_del(_settingsScreen);
        _settingsScreen = nullptr;
        for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
            _settingsItems[i] = nullptr;
        }
        _settingsTitle = nullptr;
    }
}

bool DisplayManager::isSettingsScreenVisible() const {
    return _settingsVisible;
}

void DisplayManager::createPIDScreen() {
    _pidScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_pidScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_pidScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_pidScreen, LV_SCROLLBAR_MODE_OFF);

    // Title
    _pidTitle = lv_label_create(_pidScreen);
    lv_label_set_text(_pidTitle, "PID Settings");
    lv_obj_align(_pidTitle, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_color(_pidTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_pidTitle, &lv_font_montserrat_20, 0);

    // Create labels for each item - we'll position them dynamically
    for (int i = 0; i < PID_MENU_ITEM_COUNT; i++) {
        _pidItems[i] = lv_label_create(_pidScreen);
        lv_obj_set_style_text_font(_pidItems[i], &lv_font_montserrat_20, 0);
    }
}

void DisplayManager::showPIDScreen() {
    if (!_pidScreen) {
        createPIDScreen();
    }
    lv_scr_load(_pidScreen);
    _pidVisible = true;
    _settingsVisible = false;
    updatePIDScreen(PID_MENU_MODE, false, false);
}

void DisplayManager::updatePIDScreen(PIDMenuItem selectedItem, bool editing, bool hasChanges) {
    if (!_pidScreen) return;

    // Verify all items exist
    for (int i = 0; i < PID_MENU_ITEM_COUNT; i++) {
        if (_pidItems[i] == nullptr) return;
    }

    auto& settings = SettingsManager::getInstance();

    // Build list of visible items (skip SAVE if no changes, skip AUTOTUNE always)
    int visibleItems[PID_MENU_ITEM_COUNT];
    int visibleCount = 0;
    for (int i = 0; i < PID_MENU_ITEM_COUNT; i++) {
        if (i == PID_MENU_SAVE && !hasChanges) continue;
        if (i == PID_MENU_AUTOTUNE) continue;  // Auto-tune disabled - doesn't work well for TEC
        visibleItems[visibleCount++] = i;
    }

    // Find selected item's position in visible list
    int selectedVisibleIdx = 0;
    for (int i = 0; i < visibleCount; i++) {
        if (visibleItems[i] == selectedItem) {
            selectedVisibleIdx = i;
            break;
        }
    }

    // Calculate window of items to show (max 7 items fit on screen)
    const int MAX_VISIBLE = 7;
    const int ITEM_HEIGHT = 28;
    const int START_Y = 35;

    int windowStart = 0;
    if (visibleCount > MAX_VISIBLE) {
        // Center the selected item in the window when possible
        windowStart = selectedVisibleIdx - MAX_VISIBLE / 2;
        if (windowStart < 0) windowStart = 0;
        if (windowStart > visibleCount - MAX_VISIBLE) windowStart = visibleCount - MAX_VISIBLE;
    }

    // Update all labels
    char buf[32];
    snprintf(buf, sizeof(buf), "Mode: %s", (settings.getPIDMode() == PID_ON) ? "On" : "Off");
    lv_label_set_text(_pidItems[PID_MENU_MODE], buf);

    lv_label_set_text(_pidItems[PID_MENU_AUTOTUNE], "Run Auto-tune");

    snprintf(buf, sizeof(buf), "Kp: %.2f", settings.getPIDKp());
    lv_label_set_text(_pidItems[PID_MENU_KP], buf);

    snprintf(buf, sizeof(buf), "Ki: %.2f", settings.getPIDKi());
    lv_label_set_text(_pidItems[PID_MENU_KI], buf);

    snprintf(buf, sizeof(buf), "Kd: %.2f", settings.getPIDKd());
    lv_label_set_text(_pidItems[PID_MENU_KD], buf);

    snprintf(buf, sizeof(buf), "Min: %.0f%%", settings.getPIDMinOutput());
    lv_label_set_text(_pidItems[PID_MENU_MIN], buf);

    snprintf(buf, sizeof(buf), "Max: %.0f%%", settings.getPIDMaxOutput());
    lv_label_set_text(_pidItems[PID_MENU_MAX], buf);

    lv_label_set_text(_pidItems[PID_MENU_SAVE], "Save");
    lv_label_set_text(_pidItems[PID_MENU_BACK], "< Back");

    // Hide all items first
    for (int i = 0; i < PID_MENU_ITEM_COUNT; i++) {
        lv_obj_add_flag(_pidItems[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Show and position only the visible window of items
    int yPos = START_Y;
    for (int i = windowStart; i < windowStart + MAX_VISIBLE && i < visibleCount; i++) {
        int itemIdx = visibleItems[i];
        lv_obj_clear_flag(_pidItems[itemIdx], LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(_pidItems[itemIdx], LV_ALIGN_TOP_MID, 0, yPos);

        // Set color based on selection
        if (itemIdx == selectedItem) {
            if (editing) {
                lv_obj_set_style_text_color(_pidItems[itemIdx], lv_color_hex(0x00ff00), 0);
            } else {
                lv_obj_set_style_text_color(_pidItems[itemIdx], lv_color_hex(0xffff00), 0);
            }
        } else {
            lv_obj_set_style_text_color(_pidItems[itemIdx], lv_color_hex(0x888888), 0);
        }

        yPos += ITEM_HEIGHT;
    }

    lv_refr_now(nullptr);
}

void DisplayManager::closePIDScreen() {
    // Clear old settings screen pointer (may have been invalidated by LVGL)
    _settingsScreen = nullptr;
    _settingsTitle = nullptr;
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        _settingsItems[i] = nullptr;
    }

    // Create and load new settings screen FIRST (before deleting current screen)
    createSettingsScreen();
    lv_scr_load(_settingsScreen);
    _settingsVisible = true;
    _pidVisible = false;

    // Now safe to delete PID screen (no longer active)
    if (_pidScreen) {
        lv_obj_del(_pidScreen);
        _pidScreen = nullptr;
        _pidTitle = nullptr;
        _pidContainer = nullptr;
        for (int i = 0; i < PID_MENU_ITEM_COUNT; i++) {
            _pidItems[i] = nullptr;
        }
    }
}

bool DisplayManager::isPIDScreenVisible() const {
    return _pidVisible;
}

void DisplayManager::createAutoTuneScreen() {
    _autoTuneScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_autoTuneScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_autoTuneScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_autoTuneScreen, LV_SCROLLBAR_MODE_OFF);

    // Title
    _autoTuneTitle = lv_label_create(_autoTuneScreen);
    lv_label_set_text(_autoTuneTitle, "Auto-Tune");
    lv_obj_align(_autoTuneTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_autoTuneTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_autoTuneTitle, &lv_font_montserrat_20, 0);

    // Status text
    _autoTuneStatus = lv_label_create(_autoTuneScreen);
    lv_label_set_text(_autoTuneStatus, "Starting...");
    lv_obj_align(_autoTuneStatus, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(_autoTuneStatus, lv_color_hex(0x00aaff), 0);
    lv_obj_set_style_text_font(_autoTuneStatus, &lv_font_montserrat_20, 0);

    // Progress text (cycle count)
    _autoTuneProgress = lv_label_create(_autoTuneScreen);
    lv_label_set_text(_autoTuneProgress, "Cycle 0/5");
    lv_obj_align(_autoTuneProgress, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_color(_autoTuneProgress, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(_autoTuneProgress, &lv_font_montserrat_20, 0);

    // Cancel button
    _autoTuneCancel = lv_label_create(_autoTuneScreen);
    lv_label_set_text(_autoTuneCancel, "Cancel");
    lv_obj_align(_autoTuneCancel, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(_autoTuneCancel, lv_color_hex(0xffff00), 0);
    lv_obj_set_style_text_font(_autoTuneCancel, &lv_font_montserrat_20, 0);
}

void DisplayManager::showAutoTuneScreen() {
    // Create and load auto-tune screen first
    if (!_autoTuneScreen) {
        createAutoTuneScreen();
    }
    lv_scr_load(_autoTuneScreen);
    _autoTuneVisible = true;
    _pidVisible = false;

    // Now safe to delete PID screen (no longer active)
    if (_pidScreen) {
        lv_obj_del(_pidScreen);
        _pidScreen = nullptr;
        _pidTitle = nullptr;
        for (int i = 0; i < PID_MENU_ITEM_COUNT; i++) {
            _pidItems[i] = nullptr;
        }
    }
}

void DisplayManager::updateAutoTuneScreen(int cycle, int totalCycles, const char* status) {
    if (!_autoTuneScreen) return;

    lv_label_set_text(_autoTuneStatus, status);

    char buf[32];
    snprintf(buf, sizeof(buf), "Cycle %d/%d", cycle, totalCycles);
    lv_label_set_text(_autoTuneProgress, buf);

    lv_refr_now(nullptr);
}

void DisplayManager::showAutoTuneError(const char* error) {
    if (!_autoTuneScreen) return;

    lv_label_set_text(_autoTuneStatus, error);
    lv_obj_set_style_text_color(_autoTuneStatus, lv_color_hex(0xff0000), 0);  // Red
    lv_label_set_text(_autoTuneProgress, "");
    lv_label_set_text(_autoTuneCancel, "< Back");

    lv_refr_now(nullptr);
}

void DisplayManager::closeAutoTuneScreen() {
    // Create and load PID screen first
    createPIDScreen();
    lv_scr_load(_pidScreen);
    lv_refr_now(nullptr);
    _pidVisible = true;
    _autoTuneVisible = false;

    // Now safe to delete auto-tune screen (no longer active)
    if (_autoTuneScreen) {
        lv_obj_del(_autoTuneScreen);
        _autoTuneScreen = nullptr;
        _autoTuneTitle = nullptr;
        _autoTuneStatus = nullptr;
        _autoTuneProgress = nullptr;
        _autoTuneCancel = nullptr;
    }
}

bool DisplayManager::isAutoTuneScreenVisible() const {
    return _autoTuneVisible;
}

void DisplayManager::createCurrentScreen() {
    _currentScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_currentScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_currentScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_currentScreen, LV_SCROLLBAR_MODE_OFF);

    // Title
    _currentTitle = lv_label_create(_currentScreen);
    lv_label_set_text(_currentTitle, "TEC Current");
    lv_obj_align(_currentTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_currentTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_currentTitle, &lv_font_montserrat_20, 0);

    // Current value (large)
    _currentValue = lv_label_create(_currentScreen);
    lv_label_set_text(_currentValue, "0.00 A");
    lv_obj_align(_currentValue, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(_currentValue, lv_color_hex(0x00aaff), 0);
    lv_obj_set_style_text_font(_currentValue, &lv_font_montserrat_48, 0);

    // Back button
    _currentBack = lv_label_create(_currentScreen);
    lv_label_set_text(_currentBack, "< Back");
    lv_obj_align(_currentBack, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(_currentBack, lv_color_hex(0xffff00), 0);
    lv_obj_set_style_text_font(_currentBack, &lv_font_montserrat_20, 0);
}

void DisplayManager::showCurrentScreen() {
    // Create and load current screen first
    if (!_currentScreen) {
        createCurrentScreen();
    }
    lv_scr_load(_currentScreen);
    _currentVisible = true;
    _settingsVisible = false;

    // Now safe to delete settings screen (no longer active)
    if (_settingsScreen) {
        lv_obj_del(_settingsScreen);
        _settingsScreen = nullptr;
        _settingsTitle = nullptr;
        for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
            _settingsItems[i] = nullptr;
        }
    }

    updateCurrentScreen(0.0f);
}

void DisplayManager::updateCurrentScreen(float amps) {
    if (!_currentScreen || !_currentValue) return;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f A", amps);
    lv_label_set_text(_currentValue, buf);

    lv_refr_now(nullptr);
}

void DisplayManager::closeCurrentScreen() {
    // Clear old settings screen pointer (may have been invalidated by LVGL)
    _settingsScreen = nullptr;
    _settingsTitle = nullptr;
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        _settingsItems[i] = nullptr;
    }

    // Create and load settings screen FIRST (before deleting current screen)
    createSettingsScreen();
    lv_scr_load(_settingsScreen);
    _settingsVisible = true;
    _currentVisible = false;

    // Now safe to delete current screen (no longer active)
    if (_currentScreen) {
        lv_obj_del(_currentScreen);
        _currentScreen = nullptr;
        _currentTitle = nullptr;
        _currentValue = nullptr;
        _currentBack = nullptr;
    }
}

bool DisplayManager::isCurrentScreenVisible() const {
    return _currentVisible;
}

lv_obj_t* DisplayManager::getActiveScreen() {
    return lv_scr_act();
}

void DisplayManager::createSetpointScreen() {
    _setpointScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_setpointScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_setpointScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_setpointScreen, LV_SCROLLBAR_MODE_OFF);

    // Title "Setpoint" at top
    _setpointTitle = lv_label_create(_setpointScreen);
    lv_label_set_text(_setpointTitle, "Setpoint");
    lv_obj_align(_setpointTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_setpointTitle, lv_color_hex(0xffffff), 0);  // White
    lv_obj_set_style_text_font(_setpointTitle, &lv_font_montserrat_20, 0);

    // Setpoint value (large, center) - same size as main temp display
    _setpointValue = lv_label_create(_setpointScreen);
    lv_label_set_text(_setpointValue, "0.0°");
    lv_obj_align(_setpointValue, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(_setpointValue, lv_color_hex(0xffff00), 0);  // Yellow
    lv_obj_set_style_text_font(_setpointValue, &lv_font_montserrat_96, 0);
}

void DisplayManager::showSetpointScreen(float setpointCelsius) {
    // Create and load setpoint screen first
    if (!_setpointScreen) {
        createSetpointScreen();
    }
    lv_scr_load(_setpointScreen);
    _setpointVisible = true;

    // Now safe to delete main screen (no longer active)
    if (_mainScreen) {
        lv_obj_del(_mainScreen);
        _mainScreen = nullptr;
        _tempLabel = nullptr;
        _setpointLabel = nullptr;
        _settingsIcon = nullptr;
    }

    updateSetpointScreen(setpointCelsius);
}

void DisplayManager::updateSetpointScreen(float setpointCelsius) {
    if (!_setpointScreen || !_setpointValue) return;

    auto& settings = SettingsManager::getInstance();
    float displaySetpoint = settings.toDisplayUnit(setpointCelsius);

    lv_label_set_text_fmt(_setpointValue, "%.1f°", displaySetpoint);

    lv_refr_now(nullptr);
}

void DisplayManager::closeSetpointScreen() {
    // Clear old main screen pointer (may have been invalidated by LVGL)
    _mainScreen = nullptr;
    _tempLabel = nullptr;
    _setpointLabel = nullptr;
    _settingsIcon = nullptr;

    // Create and load main screen FIRST (before deleting current screen)
    createMainScreen();
    _setpointVisible = false;

    // Now safe to delete setpoint screen (no longer active)
    if (_setpointScreen) {
        lv_obj_del(_setpointScreen);
        _setpointScreen = nullptr;
        _setpointTitle = nullptr;
        _setpointValue = nullptr;
    }
}

bool DisplayManager::isSetpointScreenVisible() const {
    return _setpointVisible;
}

void DisplayManager::createPowerScreen() {
    _powerScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_powerScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_powerScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_powerScreen, LV_SCROLLBAR_MODE_OFF);

    // Title
    _powerTitle = lv_label_create(_powerScreen);
    lv_label_set_text(_powerTitle, "TEC Power");
    lv_obj_align(_powerTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_powerTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_powerTitle, &lv_font_montserrat_20, 0);

    // Power value (large)
    _powerValue = lv_label_create(_powerScreen);
    lv_label_set_text(_powerValue, "0%");
    lv_obj_align(_powerValue, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(_powerValue, lv_color_hex(0x00aaff), 0);
    lv_obj_set_style_text_font(_powerValue, &lv_font_montserrat_48, 0);

    // Back button
    _powerBack = lv_label_create(_powerScreen);
    lv_label_set_text(_powerBack, "< Back");
    lv_obj_align(_powerBack, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(_powerBack, lv_color_hex(0xffff00), 0);
    lv_obj_set_style_text_font(_powerBack, &lv_font_montserrat_20, 0);
}

void DisplayManager::showPowerScreen() {
    // Create and load power screen first
    if (!_powerScreen) {
        createPowerScreen();
    }
    lv_scr_load(_powerScreen);
    _powerVisible = true;
    _settingsVisible = false;

    // Now safe to delete settings screen (no longer active)
    if (_settingsScreen) {
        lv_obj_del(_settingsScreen);
        _settingsScreen = nullptr;
        _settingsTitle = nullptr;
        for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
            _settingsItems[i] = nullptr;
        }
    }

    updatePowerScreen(0.0f);
}

void DisplayManager::updatePowerScreen(float powerPercent) {
    if (!_powerScreen || !_powerValue) return;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f%%", powerPercent);
    lv_label_set_text(_powerValue, buf);

    lv_refr_now(nullptr);
}

void DisplayManager::closePowerScreen() {
    // Clear old settings screen pointer (may have been invalidated by LVGL)
    _settingsScreen = nullptr;
    _settingsTitle = nullptr;
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        _settingsItems[i] = nullptr;
    }

    // Create and load settings screen FIRST (before deleting power screen)
    createSettingsScreen();
    lv_scr_load(_settingsScreen);
    _settingsVisible = true;
    _powerVisible = false;

    // Now safe to delete power screen (no longer active)
    if (_powerScreen) {
        lv_obj_del(_powerScreen);
        _powerScreen = nullptr;
        _powerTitle = nullptr;
        _powerValue = nullptr;
        _powerBack = nullptr;
    }
}

bool DisplayManager::isPowerScreenVisible() const {
    return _powerVisible;
}

void DisplayManager::createFanScreen() {
    _fanScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_fanScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_fanScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_fanScreen, LV_SCROLLBAR_MODE_OFF);

    // Fan speed setpoint at top (selectable)
    _fanSpeedLabel = lv_label_create(_fanScreen);
    lv_label_set_text(_fanSpeedLabel, "100%");
    lv_obj_align(_fanSpeedLabel, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(_fanSpeedLabel, &lv_font_montserrat_20, 0);

    // RPM display (large, center) - use 48pt to fit "2000rpm"
    _fanRPMLabel = lv_label_create(_fanScreen);
    lv_label_set_text(_fanRPMLabel, "2000rpm");
    lv_obj_align(_fanRPMLabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(_fanRPMLabel, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_fanRPMLabel, &lv_font_montserrat_48, 0);

    // Back button at bottom left (selectable) - positioned to fit circular display
    _fanBackLabel = lv_label_create(_fanScreen);
    lv_label_set_text(_fanBackLabel, "< Back");
    lv_obj_align(_fanBackLabel, LV_ALIGN_BOTTOM_LEFT, 35, -45);
    lv_obj_set_style_text_font(_fanBackLabel, &lv_font_montserrat_20, 0);

    // Options button at bottom right (selectable) - positioned to fit circular display
    _fanSmartLabel = lv_label_create(_fanScreen);
    lv_label_set_text(_fanSmartLabel, "Options >");
    lv_obj_align(_fanSmartLabel, LV_ALIGN_BOTTOM_RIGHT, -35, -45);
    lv_obj_set_style_text_font(_fanSmartLabel, &lv_font_montserrat_20, 0);
}

void DisplayManager::showFanScreen() {
    // Create and load fan screen first
    if (!_fanScreen) {
        createFanScreen();
    }
    lv_scr_load(_fanScreen);
    _fanVisible = true;
    _settingsVisible = false;

    // Now safe to delete settings screen (no longer active)
    if (_settingsScreen) {
        lv_obj_del(_settingsScreen);
        _settingsScreen = nullptr;
        _settingsTitle = nullptr;
        for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
            _settingsItems[i] = nullptr;
        }
    }

    updateFanScreen(FanController::getInstance().getAverageRPM(), 100.0f, FAN_SELECT_SPEED);
}

void DisplayManager::updateFanScreen(int rpm, float speedPercent, FanScreenSelection selection) {
    if (!_fanScreen) return;

    // Update RPM display with "rpm" suffix
    lv_label_set_text_fmt(_fanRPMLabel, "%drpm", rpm);

    // Update speed setpoint
    lv_label_set_text_fmt(_fanSpeedLabel, "%.0f%%", speedPercent);

    // Update colors based on selection
    lv_obj_set_style_text_color(_fanSpeedLabel,
        (selection == FAN_SELECT_SPEED) ? lv_color_hex(0xffff00) : lv_color_hex(0x00aaff), 0);
    lv_obj_set_style_text_color(_fanBackLabel,
        (selection == FAN_SELECT_BACK) ? lv_color_hex(0xffff00) : lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_color(_fanSmartLabel,
        (selection == FAN_SELECT_SMART) ? lv_color_hex(0xffff00) : lv_color_hex(0x888888), 0);

    lv_refr_now(nullptr);
}

void DisplayManager::closeFanScreen() {
    // Clear old settings screen pointer (may have been invalidated by LVGL)
    _settingsScreen = nullptr;
    _settingsTitle = nullptr;
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        _settingsItems[i] = nullptr;
    }

    // Create and load settings screen FIRST (before deleting fan screen)
    createSettingsScreen();
    lv_scr_load(_settingsScreen);
    _settingsVisible = true;
    _fanVisible = false;

    // Now safe to delete fan screen (no longer active)
    if (_fanScreen) {
        lv_obj_del(_fanScreen);
        _fanScreen = nullptr;
        _fanSpeedLabel = nullptr;
        _fanRPMLabel = nullptr;
        _fanBackLabel = nullptr;
        _fanSmartLabel = nullptr;
    }
}

bool DisplayManager::isFanScreenVisible() const {
    return _fanVisible;
}

void DisplayManager::createFanSpeedScreen() {
    _fanSpeedScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_fanSpeedScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_fanSpeedScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_fanSpeedScreen, LV_SCROLLBAR_MODE_OFF);

    // Title "Fan Speed" at top
    _fanSpeedTitle = lv_label_create(_fanSpeedScreen);
    lv_label_set_text(_fanSpeedTitle, "Fan Speed");
    lv_obj_align(_fanSpeedTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_fanSpeedTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_fanSpeedTitle, &lv_font_montserrat_20, 0);

    // Speed value (large, center)
    _fanSpeedValue = lv_label_create(_fanSpeedScreen);
    lv_label_set_text(_fanSpeedValue, "100%");
    lv_obj_align(_fanSpeedValue, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(_fanSpeedValue, lv_color_hex(0xffff00), 0);  // Yellow
    lv_obj_set_style_text_font(_fanSpeedValue, &lv_font_montserrat_96, 0);
}

void DisplayManager::showFanSpeedScreen(float speedPercent) {
    // Create and load fan speed screen first
    if (!_fanSpeedScreen) {
        createFanSpeedScreen();
    }
    lv_scr_load(_fanSpeedScreen);
    _fanSpeedVisible = true;

    // Now safe to delete fan screen (no longer active)
    if (_fanScreen) {
        lv_obj_del(_fanScreen);
        _fanScreen = nullptr;
        _fanSpeedLabel = nullptr;
        _fanRPMLabel = nullptr;
        _fanBackLabel = nullptr;
        _fanSmartLabel = nullptr;
    }

    updateFanSpeedScreen(speedPercent);
}

void DisplayManager::updateFanSpeedScreen(float speedPercent) {
    if (!_fanSpeedScreen || !_fanSpeedValue) return;

    lv_label_set_text_fmt(_fanSpeedValue, "%.0f%%", speedPercent);

    lv_refr_now(nullptr);
}

void DisplayManager::closeFanSpeedScreen() {
    // Clear old fan screen pointer (may have been invalidated by LVGL)
    _fanScreen = nullptr;
    _fanSpeedLabel = nullptr;
    _fanRPMLabel = nullptr;
    _fanBackLabel = nullptr;
    _fanSmartLabel = nullptr;

    // Create and load fan screen FIRST (before deleting fan speed screen)
    createFanScreen();
    lv_scr_load(_fanScreen);
    _fanVisible = true;
    _fanSpeedVisible = false;

    // Now safe to delete fan speed screen (no longer active)
    if (_fanSpeedScreen) {
        lv_obj_del(_fanSpeedScreen);
        _fanSpeedScreen = nullptr;
        _fanSpeedTitle = nullptr;
        _fanSpeedValue = nullptr;
    }
}

bool DisplayManager::isFanSpeedScreenVisible() const {
    return _fanSpeedVisible;
}

void DisplayManager::createSmartControlScreen() {
    _smartControlScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_smartControlScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_smartControlScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_smartControlScreen, LV_SCROLLBAR_MODE_OFF);

    // Title
    _smartControlTitle = lv_label_create(_smartControlScreen);
    lv_label_set_text(_smartControlTitle, "Smart Control");
    lv_obj_align(_smartControlTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_smartControlTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_smartControlTitle, &lv_font_montserrat_20, 0);

    // Smart Control toggle item
    _smartControlItems[SMART_CONTROL_TOGGLE] = lv_label_create(_smartControlScreen);
    lv_obj_align(_smartControlItems[SMART_CONTROL_TOGGLE], LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(_smartControlItems[SMART_CONTROL_TOGGLE], &lv_font_montserrat_20, 0);

    // Smart Setpoint item
    _smartControlItems[SMART_CONTROL_SETPOINT] = lv_label_create(_smartControlScreen);
    lv_obj_align(_smartControlItems[SMART_CONTROL_SETPOINT], LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_text_font(_smartControlItems[SMART_CONTROL_SETPOINT], &lv_font_montserrat_20, 0);

    // Max Fan item
    _smartControlItems[SMART_CONTROL_MAX_FAN] = lv_label_create(_smartControlScreen);
    lv_obj_align(_smartControlItems[SMART_CONTROL_MAX_FAN], LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_font(_smartControlItems[SMART_CONTROL_MAX_FAN], &lv_font_montserrat_20, 0);

    // Back item
    _smartControlItems[SMART_CONTROL_BACK] = lv_label_create(_smartControlScreen);
    lv_label_set_text(_smartControlItems[SMART_CONTROL_BACK], "< Back");
    lv_obj_align(_smartControlItems[SMART_CONTROL_BACK], LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(_smartControlItems[SMART_CONTROL_BACK], &lv_font_montserrat_20, 0);
}

void DisplayManager::showSmartControlScreen() {
    // Create and load smart control screen first
    if (!_smartControlScreen) {
        createSmartControlScreen();
    }
    lv_scr_load(_smartControlScreen);
    _smartControlVisible = true;
    _fanVisible = false;

    // Now safe to delete fan screen (no longer active)
    if (_fanScreen) {
        lv_obj_del(_fanScreen);
        _fanScreen = nullptr;
        _fanSpeedLabel = nullptr;
        _fanRPMLabel = nullptr;
        _fanBackLabel = nullptr;
        _fanSmartLabel = nullptr;
    }

    updateSmartControlScreen(SMART_CONTROL_TOGGLE, false, SettingsManager::getInstance().getSmartControlEnabled());
}

void DisplayManager::updateSmartControlScreen(SmartControlMenuItem selectedItem, bool editing, bool smartEnabled) {
    if (!_smartControlScreen) return;

    auto& settings = SettingsManager::getInstance();

    // Update toggle text
    const char* toggleText = smartEnabled ? "Smart: On" : "Smart: Off";
    lv_label_set_text(_smartControlItems[SMART_CONTROL_TOGGLE], toggleText);

    // Update setpoint text
    char buf[32];
    snprintf(buf, sizeof(buf), "Setpoint: %.0f%%", settings.getSmartSetpoint());
    lv_label_set_text(_smartControlItems[SMART_CONTROL_SETPOINT], buf);

    // Update max fan text
    snprintf(buf, sizeof(buf), "Max Fan: %.0f%%", settings.getFanSpeed());
    lv_label_set_text(_smartControlItems[SMART_CONTROL_MAX_FAN], buf);

    // Update colors based on selection and enabled state
    for (int i = 0; i < SMART_CONTROL_ITEM_COUNT; i++) {
        lv_color_t color;
        if (i == selectedItem) {
            if (editing) {
                color = lv_color_hex(0x00ff00);  // Green when editing
            } else {
                color = lv_color_hex(0xffff00);  // Yellow when selected
            }
        } else if (i == SMART_CONTROL_SETPOINT && !smartEnabled) {
            color = lv_color_hex(0x444444);  // Dark gray when disabled
        } else {
            color = lv_color_hex(0x888888);  // Gray when not selected
        }
        lv_obj_set_style_text_color(_smartControlItems[i], color, 0);
    }

    lv_refr_now(nullptr);
}

void DisplayManager::closeSmartControlScreen() {
    // Clear old fan screen pointer (may have been invalidated by LVGL)
    _fanScreen = nullptr;
    _fanSpeedLabel = nullptr;
    _fanRPMLabel = nullptr;
    _fanBackLabel = nullptr;
    _fanSmartLabel = nullptr;

    // Create and load fan screen FIRST (before deleting smart control screen)
    createFanScreen();
    lv_scr_load(_fanScreen);
    _fanVisible = true;
    _smartControlVisible = false;

    // Now safe to delete smart control screen (no longer active)
    if (_smartControlScreen) {
        lv_obj_del(_smartControlScreen);
        _smartControlScreen = nullptr;
        _smartControlTitle = nullptr;
        for (int i = 0; i < SMART_CONTROL_ITEM_COUNT; i++) {
            _smartControlItems[i] = nullptr;
        }
    }
}

bool DisplayManager::isSmartControlScreenVisible() const {
    return _smartControlVisible;
}

void DisplayManager::createFirmwareScreen() {
    _firmwareScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_firmwareScreen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(_firmwareScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_firmwareScreen, LV_SCROLLBAR_MODE_OFF);

    // Title
    _firmwareTitle = lv_label_create(_firmwareScreen);
    lv_label_set_text(_firmwareTitle, "GPIO Firmware");
    lv_obj_align(_firmwareTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(_firmwareTitle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_firmwareTitle, &lv_font_montserrat_20, 0);

    // Version item (display only)
    _firmwareItems[FIRMWARE_VERSION] = lv_label_create(_firmwareScreen);
    lv_obj_align(_firmwareItems[FIRMWARE_VERSION], LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(_firmwareItems[FIRMWARE_VERSION], &lv_font_montserrat_20, 0);

    // Update GPIO FW item
    _firmwareItems[FIRMWARE_UPDATE] = lv_label_create(_firmwareScreen);
    lv_label_set_text(_firmwareItems[FIRMWARE_UPDATE], "Update GPIO FW");
    lv_obj_align(_firmwareItems[FIRMWARE_UPDATE], LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_text_font(_firmwareItems[FIRMWARE_UPDATE], &lv_font_montserrat_20, 0);

    // Restore Original item
    _firmwareItems[FIRMWARE_RESTORE] = lv_label_create(_firmwareScreen);
    lv_label_set_text(_firmwareItems[FIRMWARE_RESTORE], "Restore Original");
    lv_obj_align(_firmwareItems[FIRMWARE_RESTORE], LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_font(_firmwareItems[FIRMWARE_RESTORE], &lv_font_montserrat_20, 0);

    // Back item
    _firmwareItems[FIRMWARE_BACK] = lv_label_create(_firmwareScreen);
    lv_label_set_text(_firmwareItems[FIRMWARE_BACK], "< Back");
    lv_obj_align(_firmwareItems[FIRMWARE_BACK], LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(_firmwareItems[FIRMWARE_BACK], &lv_font_montserrat_20, 0);

    // Progress label (hidden by default, shown during flashing)
    _firmwareProgress = lv_label_create(_firmwareScreen);
    lv_label_set_text(_firmwareProgress, "");
    lv_obj_align(_firmwareProgress, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_color(_firmwareProgress, lv_color_hex(0x00aaff), 0);
    lv_obj_set_style_text_font(_firmwareProgress, &lv_font_montserrat_20, 0);
    lv_obj_add_flag(_firmwareProgress, LV_OBJ_FLAG_HIDDEN);

    // Status label (hidden by default, shown during flashing)
    _firmwareStatus = lv_label_create(_firmwareScreen);
    lv_label_set_text(_firmwareStatus, "");
    lv_obj_align(_firmwareStatus, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(_firmwareStatus, lv_color_hex(0xffff00), 0);
    lv_obj_set_style_text_font(_firmwareStatus, &lv_font_montserrat_20, 0);
    lv_obj_add_flag(_firmwareStatus, LV_OBJ_FLAG_HIDDEN);
}

void DisplayManager::showFirmwareScreen() {
    // Create and load firmware screen first
    if (!_firmwareScreen) {
        createFirmwareScreen();
    }
    lv_scr_load(_firmwareScreen);
    _firmwareVisible = true;
    _settingsVisible = false;

    // Now safe to delete settings screen (no longer active)
    if (_settingsScreen) {
        lv_obj_del(_settingsScreen);
        _settingsScreen = nullptr;
        _settingsTitle = nullptr;
        for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
            _settingsItems[i] = nullptr;
        }
    }

    updateFirmwareScreen(FIRMWARE_VERSION, 0);
}

void DisplayManager::updateFirmwareScreen(FirmwareMenuItem selectedItem, uint8_t version) {
    if (!_firmwareScreen) return;

    // Update version display
    char buf[32];
    snprintf(buf, sizeof(buf), "Version: %d", version);
    lv_label_set_text(_firmwareItems[FIRMWARE_VERSION], buf);

    // Show menu items, hide progress/status
    for (int i = 0; i < FIRMWARE_ITEM_COUNT; i++) {
        lv_obj_clear_flag(_firmwareItems[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(_firmwareProgress, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_firmwareStatus, LV_OBJ_FLAG_HIDDEN);

    // Update colors based on selection
    for (int i = 0; i < FIRMWARE_ITEM_COUNT; i++) {
        if (i == selectedItem) {
            lv_obj_set_style_text_color(_firmwareItems[i], lv_color_hex(0xffff00), 0);  // Yellow
        } else if (i == FIRMWARE_VERSION) {
            lv_obj_set_style_text_color(_firmwareItems[i], lv_color_hex(0x00aaff), 0);  // Cyan for version
        } else {
            lv_obj_set_style_text_color(_firmwareItems[i], lv_color_hex(0x888888), 0);  // Gray
        }
    }

    lv_refr_now(nullptr);
}

void DisplayManager::showFlashingProgress(int currentPage, int totalPages, const char* status) {
    if (!_firmwareScreen) return;

    // Hide menu items
    for (int i = 0; i < FIRMWARE_ITEM_COUNT; i++) {
        lv_obj_add_flag(_firmwareItems[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Show progress and status
    lv_obj_clear_flag(_firmwareProgress, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_firmwareStatus, LV_OBJ_FLAG_HIDDEN);

    // Update text
    lv_label_set_text(_firmwareStatus, status);

    char buf[32];
    snprintf(buf, sizeof(buf), "Page %d/%d", currentPage, totalPages);
    lv_label_set_text(_firmwareProgress, buf);

    lv_refr_now(nullptr);
}

void DisplayManager::closeFirmwareScreen() {
    // Clear old settings screen pointer (may have been invalidated by LVGL)
    _settingsScreen = nullptr;
    _settingsTitle = nullptr;
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        _settingsItems[i] = nullptr;
    }

    // Create and load settings screen FIRST (before deleting firmware screen)
    createSettingsScreen();
    lv_scr_load(_settingsScreen);
    _settingsVisible = true;
    _firmwareVisible = false;

    // Now safe to delete firmware screen (no longer active)
    if (_firmwareScreen) {
        lv_obj_del(_firmwareScreen);
        _firmwareScreen = nullptr;
        _firmwareTitle = nullptr;
        for (int i = 0; i < FIRMWARE_ITEM_COUNT; i++) {
            _firmwareItems[i] = nullptr;
        }
        _firmwareProgress = nullptr;
        _firmwareStatus = nullptr;
    }
}

bool DisplayManager::isFirmwareScreenVisible() const {
    return _firmwareVisible;
}
