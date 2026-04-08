#pragma once
#include <Arduino.h>

class SleepManager {
public:
    static constexpr uint8_t  SLEEP_BRIGHTNESS  = 0;
    static constexpr uint32_t SLEEP_TIMEOUT_MS  = 60000;
    static constexpr int      SLEEP_HOUR_START  = 0;
    static constexpr int      SLEEP_HOUR_END    = 7;

    void begin(uint8_t normalBrightness);
    void onActivity();
    void tick();
    bool isSleeping() const { return _sleeping; }

private:
    bool     _sleeping         = false;
    uint8_t  _normalBrightness = 128;
    uint32_t _lastActivityMs   = 0;

    bool inSleepWindow() const;
};

extern SleepManager sleepManager;
