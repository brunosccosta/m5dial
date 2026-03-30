#include <esp_log.h>
#include "RestScreen.h"
#include "../AppState.h"
#include "fonts/fa_icons.h"

static const char* TAG = "REST";

RestScreen restScreen;


// Minimal mode icon lookup — mirrors ACControlScreen but only needs the icon glyph
static const char* modeIcon(const char* mode) {
    if (strcmp(mode, "heat")     == 0) return FA_FIRE;
    if (strcmp(mode, "cool")     == 0) return FA_SNOWFLAKE;
    if (strcmp(mode, "auto")     == 0) return FA_ARROWS_ROTATE;
    if (strcmp(mode, "fan_only") == 0) return FA_FAN;
    if (strcmp(mode, "dry")      == 0) return FA_DROPLET;
    return FA_POWER_OFF;
}

// --- Lifecycle ---

void RestScreen::setOnWake(std::function<void()> cb) {
    _onWake = cb;
}

void RestScreen::init() {
    if (_initialized) return;
    _initialized = true;

    _lvScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_lvScreen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(_lvScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Current weather — condition + temp, prominent
    _currentTempLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_currentTempLabel, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(_currentTempLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_currentTempLabel, LV_ALIGN_CENTER, 0, -65);

    _conditionLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_conditionLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_conditionLabel, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(_conditionLabel, LV_ALIGN_CENTER, 0, -40);

    // 1h forecast — smaller, grey
    _forecastLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_forecastLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_forecastLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(_forecastLabel, LV_ALIGN_CENTER, 0, -14);

    // Outside temp — small, dim
    _outsideTempLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_outsideTempLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_outsideTempLabel, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_align(_outsideTempLabel, LV_ALIGN_CENTER, 0, +14);

    // AC status — icon above, state below, left side
    _acIconLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_acIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_acIconLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_acIconLabel, LV_ALIGN_CENTER, -45, +42);

    _acStateLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_acStateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_acStateLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(_acStateLabel, LV_ALIGN_CENTER, -45, +60);

    // Heater status — icon above, state below, right side
    _heaterIconLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_heaterIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_heaterIconLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_heaterIconLabel, LV_ALIGN_CENTER, +45, +42);

    _heaterStateLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_heaterStateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_heaterStateLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(_heaterStateLabel, LV_ALIGN_CENTER, +45, +60);

    ESP_LOGI(TAG, "init complete");
}

void RestScreen::show() {
    updateDisplay();
    lv_scr_load(_lvScreen);
    lv_refr_now(NULL);
}

void RestScreen::onEncoder(int delta) {
    wake();
}

void RestScreen::onButton() {
    wake();
}

void RestScreen::refresh() {
    updateDisplay();
}

void RestScreen::wake() {
    ESP_LOGI(TAG, "wake");
    if (_onWake) _onWake();
}

// --- Display ---

void RestScreen::updateDisplay() {
    char buf[48];

    // Current weather
    WeatherState& w = appState.weather;
    if (w.valid) {
        snprintf(buf, sizeof(buf), "%.0f°  %.0f°", w.temperature, w.feelsLike);
        lv_label_set_text(_currentTempLabel, buf);
        lv_label_set_text(_conditionLabel, w.condition);
        snprintf(buf, sizeof(buf), "outside  %.1f°  %.0f%%", w.outdoorTemp, w.outdoorHumidity);
        lv_label_set_text(_outsideTempLabel, buf);
    } else {
        lv_label_set_text(_currentTempLabel, "--°");
        lv_label_set_text(_conditionLabel, "---");
        lv_label_set_text(_outsideTempLabel, "outside  --");
    }
    lv_label_set_text(_forecastLabel, ""); // forecast deferred

    // Re-align after text update
    lv_obj_align(_currentTempLabel,  LV_ALIGN_CENTER, 0, -65);
    lv_obj_align(_conditionLabel,    LV_ALIGN_CENTER, 0, -40);
    lv_obj_align(_forecastLabel,     LV_ALIGN_CENTER, 0, -14);
    lv_obj_align(_outsideTempLabel,  LV_ALIGN_CENTER, 0, +14);

    // AC status
    ACState& ac = appState.acs[0];
    if (ac.valid) {
        lv_label_set_text(_acIconLabel, modeIcon(ac.mode));
        snprintf(buf, sizeof(buf), "%s %.0f°", ac.mode, ac.target_temp);
        lv_label_set_text(_acStateLabel, buf);
    } else {
        lv_label_set_text(_acIconLabel, FA_POWER_OFF);
        lv_label_set_text(_acStateLabel, "---");
    }

    // Heater status
    ACState& heater = appState.acs[1];
    if (heater.valid) {
        lv_label_set_text(_heaterIconLabel, FA_FIRE);
        lv_label_set_text(_heaterStateLabel, strcmp(heater.mode, "off") == 0 ? "OFF" : "ON");
    } else {
        lv_label_set_text(_heaterIconLabel, FA_FIRE);
        lv_label_set_text(_heaterStateLabel, "---");
    }

    lv_obj_align(_acIconLabel,      LV_ALIGN_CENTER, -45, +42);
    lv_obj_align(_acStateLabel,     LV_ALIGN_CENTER, -45, +60);
    lv_obj_align(_heaterIconLabel,  LV_ALIGN_CENTER, +45, +42);
    lv_obj_align(_heaterStateLabel, LV_ALIGN_CENTER, +45, +60);
}
