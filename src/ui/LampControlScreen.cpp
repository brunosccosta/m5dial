#include <esp_log.h>
#include "LampControlScreen.h"
#include "../AppState.h"
#include "ScreenManager.h"
#include "Theme.h"

static const char* TAG = "LAMP";

void LampControlScreen::setLampIndex(int idx) {
    _lampIdx = idx;
}

void LampControlScreen::init() {
    if (_initialized) return;
    _initialized = true;

    _lvScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_lvScreen, lv_color_hex(Theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);

    _nameLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_nameLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_nameLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_nameLabel, LV_ALIGN_CENTER, 0, -50);

    _stateLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_stateLabel, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(_stateLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_stateLabel, LV_ALIGN_CENTER, 0, 0);

    _brightnessLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_brightnessLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_brightnessLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_brightnessLabel, LV_ALIGN_CENTER, 0, 45);

    ESP_LOGI(TAG, "init complete");
}

void LampControlScreen::show() {
    updateDisplay();
    lv_scr_load(_lvScreen);
    lv_refr_now(NULL);
}

void LampControlScreen::onEncoder(int delta) {
    LampState& lamp = appState.lamps[_lampIdx];
    int brightness = lamp.brightness + delta * 10;
    if (brightness < 0)   brightness = 0;
    if (brightness > 255) brightness = 255;
    lamp.brightness = (uint8_t)brightness;
    if (brightness > 0) lamp.on = true;
    updateDisplay();
}

void LampControlScreen::onButton() {
    screenManager.pop();
}

void LampControlScreen::refresh() {
    updateDisplay();
}

void LampControlScreen::updateDisplay() {
    LampState& lamp = appState.lamps[_lampIdx];

    lv_label_set_text(_nameLabel, lamp.name);
    lv_obj_align(_nameLabel, LV_ALIGN_CENTER, 0, -50);

    lv_label_set_text(_stateLabel, lamp.on ? "ON" : "OFF");
    lv_obj_set_style_text_color(_stateLabel,
        lamp.on ? lv_color_hex(Theme::TEXT_PRIMARY) : lv_color_hex(Theme::SURFACE_FAINT),
        LV_PART_MAIN);
    lv_obj_align(_stateLabel, LV_ALIGN_CENTER, 0, 0);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d%%", (lamp.brightness * 100) / 255);
    lv_label_set_text(_brightnessLabel, buf);
    lv_obj_align(_brightnessLabel, LV_ALIGN_CENTER, 0, 45);

    lv_refr_now(NULL);
}
