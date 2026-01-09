#include "InputController.h"
#include <M5Dial.h>
#include "driver/pcnt.h"
#include "driver/gpio.h"

// M5Dial button is on GPIO 42
static constexpr gpio_num_t BUTTON_GPIO = GPIO_NUM_42;

// Static member definitions
volatile bool InputController::_buttonPressed = false;
volatile unsigned long InputController::_lastButtonTime = 0;

// PCNT hardware encoder - fixes M5Dial library bug with ESP32-S3 GPIO 40/41
static void initPCNTEncoder() {
    pcnt_config_t enc_config = {
        .pulse_gpio_num = GPIO_NUM_40,  // Rotary Encoder Chan A
        .ctrl_gpio_num = GPIO_NUM_41,   // Rotary Encoder Chan B
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_REVERSE,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .counter_h_lim = INT16_MAX,
        .counter_l_lim = INT16_MIN,
        .unit = PCNT_UNIT_0,
        .channel = PCNT_CHANNEL_0,
    };
    pcnt_unit_config(&enc_config);

    enc_config.pulse_gpio_num = GPIO_NUM_41;
    enc_config.ctrl_gpio_num = GPIO_NUM_40;
    enc_config.channel = PCNT_CHANNEL_1;
    enc_config.pos_mode = PCNT_COUNT_DEC;
    enc_config.neg_mode = PCNT_COUNT_INC;
    pcnt_unit_config(&enc_config);

    pcnt_set_filter_value(PCNT_UNIT_0, 250);
    pcnt_filter_enable(PCNT_UNIT_0);

    gpio_pullup_en(GPIO_NUM_40);
    gpio_pullup_en(GPIO_NUM_41);

    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
}

static int16_t getPCNTEncoder() {
    int16_t count;
    pcnt_get_counter_value(PCNT_UNIT_0, &count);
    return count;
}

// Button interrupt handler
void IRAM_ATTR InputController::buttonISR() {
    unsigned long now = millis();
    if (now - _lastButtonTime > DEBOUNCE_MS) {
        _buttonPressed = true;
        _lastButtonTime = now;
    }
}

InputController& InputController::getInstance() {
    static InputController instance;
    return instance;
}

void InputController::begin() {
    initPCNTEncoder();
    _lastActionPos = 0;
    _buttonPressed = false;
    _lastButtonTime = 0;

    // Setup button interrupt (active low, trigger on falling edge)
    pinMode(BUTTON_GPIO, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_GPIO), buttonISR, FALLING);
}

void InputController::update() {
    // Button is now handled by interrupt, nothing to poll
}

int InputController::getEncoderDelta() {
    int32_t currentPos = getPCNTEncoder();
    int32_t diff = currentPos - _lastActionPos;

    // Count how many full detents have accumulated
    int detents = diff / PULSES_PER_DETENT;

    if (detents != 0) {
        // Consume only the full detents (keep remainder for next time)
        _lastActionPos += detents * PULSES_PER_DETENT;

        // Calculate acceleration multiplier based on time since last detent
        unsigned long now = millis();
        unsigned long elapsed = now - _lastDetentTime;
        _lastDetentTime = now;

        int multiplier = 1;
        if (elapsed <= ACCEL_FAST_MS) {
            multiplier = 8;
        } else if (elapsed <= ACCEL_MEDIUM_MS) {
            multiplier = 4;
        } else if (elapsed <= ACCEL_SLOW_MS) {
            multiplier = 2;
        }

        // Return detents * multiplier (both count and speed matter)
        return detents * multiplier;
    }
    return 0;
}

bool InputController::wasButtonPressed() {
    bool pressed = _buttonPressed;
    _buttonPressed = false;
    return pressed;
}

void InputController::requeueButtonPress() {
    _buttonPressed = true;
}

void InputController::playTone(uint16_t freq, uint16_t duration) {
    M5Dial.Speaker.tone(freq, duration);
}

void InputController::playNavigationBeep() {
    playTone(6000, 10);
}

void InputController::playEnterBeep() {
    playTone(4000, 50);  // Lower freq, longer duration for better audibility
}

void InputController::playExitBeep() {
    playTone(4000, 20);
}

void InputController::playToggleBeep() {
    playTone(7000, 20);
}
