#include <esp_log.h>
#include <lvgl.h>
#include "ScreenManager.h"

static const char* TAG = "NAV";

static constexpr uint32_t SCREEN_ANIM_MS = 200;

ScreenManager screenManager;

void ScreenManager::push(Screen* screen) {
    if (_depth >= MAX_DEPTH) {
        ESP_LOGE(TAG, "stack overflow");
        return;
    }
    screen->init();
    _stack[_depth++] = screen;
    screen->show();
    lv_scr_load_anim(screen->lvScreen(), LV_SCR_LOAD_ANIM_FADE_IN, SCREEN_ANIM_MS, 0, false);
    ESP_LOGD(TAG, "push depth=%d", _depth);
}

void ScreenManager::pop() {
    if (_depth <= 1) {
        ESP_LOGW(TAG, "already at root");
        return;
    }
    _depth--;
    Screen* screen = _stack[_depth - 1];
    screen->show();
    lv_scr_load_anim(screen->lvScreen(), LV_SCR_LOAD_ANIM_FADE_IN, SCREEN_ANIM_MS, 0, false);
    ESP_LOGD(TAG, "pop depth=%d", _depth);
}

void ScreenManager::onEncoder(int delta) {
    if (_depth > 0) _stack[_depth - 1]->onEncoder(delta);
}

void ScreenManager::onButton() {
    if (_depth > 0) _stack[_depth - 1]->onButton();
}

void ScreenManager::onTouch() {
    if (_depth > 0) _stack[_depth - 1]->onTouch();
}

void ScreenManager::onSwipe(int dir) {
    if (_depth > 0) _stack[_depth - 1]->onSwipe(dir);
}

void ScreenManager::refresh() {
    if (_depth > 0) _stack[_depth - 1]->refresh();
}

void ScreenManager::tick() {
    if (_depth > 0) _stack[_depth - 1]->tick();
}
