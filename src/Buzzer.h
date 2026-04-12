#pragma once
#include <Arduino.h>

class Buzzer {
public:
    void tick();     // encoder step
    void tap();      // screen tap / touch confirm
    void confirm();  // button confirm action
    void wake();     // display wake from sleep
    void rfid();     // RFID tag scanned

    void mute(bool m) { _muted = m; }
    bool isMuted()    { return _muted; }

private:
    bool _muted = false;

    // Tone parameters — all tuning in one place
    static constexpr uint32_t TICK_FREQ    = 220;  // Hz
    static constexpr uint32_t TICK_MS      = 8;

    static constexpr uint32_t TAP_FREQ     = 300;  // Hz
    static constexpr uint32_t TAP_MS       = 40;

    static constexpr uint32_t CONFIRM_FREQ = 520;  // Hz
    static constexpr uint32_t CONFIRM_MS   = 60;

    static constexpr uint32_t WAKE_FREQ    = 440;  // Hz
    static constexpr uint32_t WAKE_MS      = 30;

    static constexpr uint32_t RFID_FREQ    = 700;  // Hz
    static constexpr uint32_t RFID_MS      = 60;
};

extern Buzzer buzzer;
