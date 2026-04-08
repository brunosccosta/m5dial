#include "SleepManager.h"
#include <M5Dial.h>
#include <esp_log.h>
#include <time.h>

static const char* TAG = "SLEEP";

SleepManager sleepManager;

void SleepManager::begin(uint8_t normalBrightness) {
    _normalBrightness = normalBrightness;
    _lastActivityMs   = millis();
    _sleeping         = false;
}

void SleepManager::onActivity() {
    _lastActivityMs = millis();
    if (_sleeping) {
        _sleeping = false;
        M5Dial.Display.setBrightness(_normalBrightness);
        ESP_LOGI(TAG, "woke up");
    }
}

void SleepManager::tick() {
    if (_sleeping) return; // already asleep — nothing to check

    // Guard: skip if clock not synced
    time_t now = time(nullptr);
    if (now < 1577836800LL) return; // before 2020-01-01

    if (!inSleepWindow()) return;

    uint32_t idleMs = millis() - _lastActivityMs;
    if (idleMs >= SLEEP_TIMEOUT_MS) {
        _sleeping = true;
        M5Dial.Display.setBrightness(SLEEP_BRIGHTNESS);
        ESP_LOGI(TAG, "display off (idle %lus)", idleMs / 1000);
    }
}

bool SleepManager::inSleepWindow() const {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    return t.tm_hour >= SLEEP_HOUR_START && t.tm_hour < SLEEP_HOUR_END;
}
