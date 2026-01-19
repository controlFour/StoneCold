#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <stdint.h>

class InputController {
public:
    static InputController& getInstance();

    void begin();
    void update();  // Call in loop() after M5Dial.update()

    // Returns number of detents rotated since last call (positive = CW, negative = CCW)
    int getEncoderDelta();

    // Returns true if button was pressed since last call
    bool wasButtonPressed();

    // Re-queue a button press (used when pre-checking for screen transitions)
    void requeueButtonPress();

    // Audio feedback
    void playTone(uint16_t freq, uint16_t duration);
    void playNavigationBeep();
    void playEnterBeep();
    void playExitBeep();
    void playToggleBeep();

    // ISR callback (public so ISR can access it)
    static void buttonISR();

private:
    InputController() = default;
    InputController(const InputController&) = delete;
    InputController& operator=(const InputController&) = delete;

    int32_t _lastActionPos = 0;
    unsigned long _lastDetentTime = 0;
    static volatile bool _buttonPressed;
    static volatile unsigned long _lastButtonTime;

    // Pulses needed to trigger one step (M5Dial generates ~4 per click)
    static constexpr int PULSES_PER_DETENT = 4;
    static constexpr unsigned long DEBOUNCE_MS = 50;

    // Encoder acceleration thresholds (ms between detents)
    static constexpr unsigned long ACCEL_SLOW_MS = 200;    // > this = 1x
    static constexpr unsigned long ACCEL_MEDIUM_MS = 100;  // > this = 2x
    static constexpr unsigned long ACCEL_FAST_MS = 50;     // > this = 4x
                                                           // <= this = 8x
};

#endif
