#include <esp_log.h>
#include "ScreenManager.h"

static const char* TAG = "NAV";

ScreenManager screenManager;

void ScreenManager::push(Screen* screen) {
    if (_depth >= MAX_DEPTH) {
        ESP_LOGE(TAG, "stack overflow");
        return;
    }
    screen->init();
    _stack[_depth++] = screen;
    screen->show();
    ESP_LOGD(TAG, "push depth=%d", _depth);
}

void ScreenManager::pop() {
    if (_depth <= 1) {
        ESP_LOGW(TAG, "already at root");
        return;
    }
    _depth--;
    _stack[_depth - 1]->show();
    ESP_LOGD(TAG, "pop depth=%d", _depth);
}

void ScreenManager::onEncoder(int delta) {
    if (_depth > 0) _stack[_depth - 1]->onEncoder(delta);
}

void ScreenManager::onButton() {
    if (_depth > 0) _stack[_depth - 1]->onButton();
}

void ScreenManager::refresh() {
    if (_depth > 0) _stack[_depth - 1]->refresh();
}

void ScreenManager::tick() {
    if (_depth > 0) _stack[_depth - 1]->tick();
}
