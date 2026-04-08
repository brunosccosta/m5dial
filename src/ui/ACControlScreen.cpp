#include <Arduino.h>
#include <esp_log.h>
#include "ACControlScreen.h"
#include "ConfirmScreen.h"
#include "ScreenManager.h"
#include "../AppState.h"
#include "../ha/HAClient.h"
#include "fonts/fa_icons.h"
#include "Theme.h"

static const char* TAG = "AC";

static const char* modeIcon(const char* mode) {
    if (strcmp(mode, "heat")     == 0) return FA_FIRE;
    if (strcmp(mode, "cool")     == 0) return FA_SNOWFLAKE;
    if (strcmp(mode, "auto")      == 0) return FA_ARROWS_ROTATE;
    if (strcmp(mode, "heat_cool") == 0) return FA_ARROWS_ROTATE;
    if (strcmp(mode, "fan_only") == 0) return FA_FAN;
    if (strcmp(mode, "dry")      == 0) return FA_DROPLET;
    return FA_POWER_OFF;
}

static const char* modeLabel(const char* mode) {
    if (strcmp(mode, "heat")     == 0) return "HEAT";
    if (strcmp(mode, "cool")     == 0) return "COOL";
    if (strcmp(mode, "auto")      == 0) return "AUTO";
    if (strcmp(mode, "heat_cool") == 0) return "AUTO";
    if (strcmp(mode, "fan_only") == 0) return "FAN";
    if (strcmp(mode, "dry")      == 0) return "DRY";
    return "OFF";
}

static lv_color_t arcColor(const char* mode) {
    if (strcmp(mode, "heat")     == 0) return lv_color_hex(Theme::AC_MODE_HEAT);
    if (strcmp(mode, "cool")     == 0) return lv_color_hex(Theme::AC_MODE_COOL);
    if (strcmp(mode, "auto")      == 0) return lv_color_hex(Theme::AC_MODE_AUTO);
    if (strcmp(mode, "heat_cool") == 0) return lv_color_hex(Theme::AC_MODE_AUTO);
    if (strcmp(mode, "fan_only") == 0) return lv_color_hex(Theme::AC_MODE_FAN);
    if (strcmp(mode, "dry")      == 0) return lv_color_hex(Theme::AC_MODE_DRY);
    return lv_color_hex(Theme::AC_MODE_OFF);
}

static lv_color_t tempColor(float temp) {
    if (temp < 18.0f) return lv_color_hex(Theme::TEMP_COLD);
    if (temp < 21.0f) return lv_color_hex(Theme::TEMP_MID);
    return                   lv_color_hex(Theme::TEMP_WARM);
}

// --- Setup ---

void ACControlScreen::setAC(ACState* ac) {
    _ac = ac;
}

// --- Lifecycle ---

void ACControlScreen::init() {
    if (_initialized) return;
    _initialized = true;

    _lvScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_lvScreen, lv_color_hex(Theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(_lvScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Inner arc — current temp
    _arcInner = lv_arc_create(_lvScreen);
    lv_obj_set_size(_arcInner, 190, 190);
    lv_obj_center(_arcInner);
    lv_arc_set_bg_start_angle(_arcInner, 185);
    lv_arc_set_bg_end_angle(_arcInner, 355);
    lv_arc_set_range(_arcInner, 10, 30);
    lv_arc_set_value(_arcInner, 10);
    lv_obj_set_style_arc_color(_arcInner, lv_color_hex(Theme::SURFACE_DARK), LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arcInner, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arcInner, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(_arcInner, 160, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_arcInner, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(_arcInner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(_arcInner, LV_OBJ_FLAG_CLICKABLE);

    // Outer arc — target temp
    _arc = lv_arc_create(_lvScreen);
    lv_obj_set_size(_arc, 220, 220);
    lv_obj_center(_arc);
    lv_arc_set_bg_start_angle(_arc, 185);
    lv_arc_set_bg_end_angle(_arc, 355);
    lv_arc_set_range(_arc, 10, 30);
    lv_arc_set_value(_arc, 21);
    lv_obj_set_style_arc_color(_arc, lv_color_hex(Theme::SURFACE), LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(_arc, LV_OBJ_FLAG_CLICKABLE);

    // Current temp
    _currentTempLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_currentTempLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_currentTempLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_currentTempLabel, "--.-\xC2\xB0");
    lv_obj_align(_currentTempLabel, LV_ALIGN_CENTER, 0, -46);

    // Target temp — large, center
    _targetTempLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_targetTempLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(_targetTempLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_targetTempLabel, "--\xC2\xB0");
    lv_obj_align(_targetTempLabel, LV_ALIGN_CENTER, 0, -2);

    // Mode pill — clickable container with icon + label
    _modeContainer = lv_obj_create(_lvScreen);
    lv_obj_set_size(_modeContainer, 90, 36);
    lv_obj_set_style_bg_opa(_modeContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_modeContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_modeContainer, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_modeContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_modeContainer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(_modeContainer, LV_ALIGN_CENTER, 0, 47);
    lv_obj_add_event_cb(_modeContainer, onModeClick, LV_EVENT_CLICKED, this);

    _modeIconLabel = lv_label_create(_modeContainer);
    lv_obj_set_style_text_font(_modeIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_modeIconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_modeIconLabel, FA_POWER_OFF);
    lv_obj_align(_modeIconLabel, LV_ALIGN_LEFT_MID, 0, 0);

    _modeTextLabel = lv_label_create(_modeContainer);
    lv_obj_set_style_text_font(_modeTextLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_modeTextLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_modeTextLabel, "OFF");
    lv_obj_align(_modeTextLabel, LV_ALIGN_RIGHT_MID, 0, 0);

    ESP_LOGI(TAG, "init complete");
}

void ACControlScreen::show() {
    if (!_ac) return;

    _pendingTemp = _ac->target_temp;
    strncpy(_pendingMode, _ac->mode, sizeof(_pendingMode) - 1);
    _pendingMode[sizeof(_pendingMode) - 1] = '\0';

    _originalTemp = _pendingTemp;
    strncpy(_originalMode, _pendingMode, sizeof(_originalMode));

    _lastActivityMs = millis();
    updateDisplay();
}

void ACControlScreen::tick() {
    if (millis() - _lastActivityMs >= INACTIVITY_MS) {
        ESP_LOGI(TAG, "inactivity timeout");
        _lastActivityMs = millis();
        screenManager.pop();
    }
}

// --- Input ---

void ACControlScreen::onEncoder(int delta) {
    _lastActivityMs = millis();
    _pendingTemp -= delta * 0.5f;
    if (_pendingTemp < 10.0f) _pendingTemp = 10.0f;
    if (_pendingTemp > 30.0f) _pendingTemp = 30.0f;

    lv_arc_set_value(_arc, (int16_t)_pendingTemp);
    char buf[8];
    snprintf(buf, sizeof(buf), "%.1f\xC2\xB0", _pendingTemp);
    lv_label_set_text(_targetTempLabel, buf);
    lv_refr_now(NULL);
}

void ACControlScreen::toggleMode() {
    _lastActivityMs = millis();

    static const char* FALLBACK[] = { "heat", "cool", "auto", "fan_only", "dry", "off" };
    static constexpr int FALLBACK_COUNT = 6;

    bool useDevice = _ac && _ac->modeCount > 0;
    int  count     = useDevice ? _ac->modeCount : FALLBACK_COUNT;

    int idx = 0;
    for (int i = 0; i < count; i++) {
        const char* m = useDevice ? _ac->availableModes[i] : FALLBACK[i];
        if (strcmp(_pendingMode, m) == 0) { idx = i; break; }
    }
    idx = (idx + 1) % count;

    const char* next = useDevice ? _ac->availableModes[idx] : FALLBACK[idx];
    strncpy(_pendingMode, next, sizeof(_pendingMode) - 1);
    _pendingMode[sizeof(_pendingMode) - 1] = '\0';

    updateDisplay();
}

void ACControlScreen::onModeClick(lv_event_t* e) {
    static_cast<ACControlScreen*>(lv_event_get_user_data(e))->toggleMode();
}

void ACControlScreen::onButton() {
    _lastActivityMs = millis();
    if (!hasChanges()) {
        screenManager.pop();
        return;
    }
    confirmScreen.setup("Save changes?",
        [this]() {
            strncpy(_ac->mode, _pendingMode, sizeof(_ac->mode));
            _ac->target_temp = _pendingTemp;
            appState.dirty = true;
            haClient.sendACTemperature(_ac->entity_id, _pendingTemp);
            haClient.sendACMode(_ac->entity_id, _pendingMode);
            screenManager.pop(); // ConfirmScreen
            screenManager.pop(); // ACControlScreen → MenuScreen
        },
        [this]() {
            screenManager.pop(); // ConfirmScreen
            screenManager.pop(); // ACControlScreen → MenuScreen
        }
    );
    screenManager.push(&confirmScreen);
}

void ACControlScreen::refresh() {
    if (!_ac) return;
    // Only update current temp from live state; don't overwrite pending edits
    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f\xC2\xB0", _ac->current_temp);
    lv_label_set_text(_currentTempLabel, buf);
    lv_arc_set_value(_arcInner, (int16_t)_ac->current_temp);
    lv_obj_set_style_arc_color(_arcInner, tempColor(_ac->current_temp), LV_PART_INDICATOR);
    lv_refr_now(NULL);
}

// --- Helpers ---

bool ACControlScreen::hasChanges() const {
    if (!_ac) return false;
    if (fabsf(_pendingTemp - _originalTemp) > 0.09f) return true;
    if (strcmp(_pendingMode, _originalMode) != 0) return true;
    return false;
}

void ACControlScreen::updateDisplay() {
    lv_arc_set_value(_arc, (int16_t)_pendingTemp);
    lv_obj_set_style_arc_color(_arc, arcColor(_pendingMode), LV_PART_INDICATOR);

    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f\xC2\xB0", _pendingTemp);
    lv_label_set_text(_targetTempLabel, buf);

    if (_ac && _ac->valid) {
        snprintf(buf, sizeof(buf), "%.1f\xC2\xB0", _ac->current_temp);
        lv_label_set_text(_currentTempLabel, buf);
        lv_arc_set_value(_arcInner, (int16_t)_ac->current_temp);
        lv_obj_set_style_arc_color(_arcInner, tempColor(_ac->current_temp), LV_PART_INDICATOR);
    }

    lv_label_set_text(_modeIconLabel, modeIcon(_pendingMode));
    lv_label_set_text(_modeTextLabel, modeLabel(_pendingMode));

    lv_obj_align(_currentTempLabel, LV_ALIGN_CENTER, 0, -46);
    lv_obj_align(_targetTempLabel,  LV_ALIGN_CENTER, 0, -2);
    lv_obj_align(_modeIconLabel,    LV_ALIGN_LEFT_MID,  0, 0);
    lv_obj_align(_modeTextLabel,    LV_ALIGN_RIGHT_MID, 0, 0);

    lv_refr_now(NULL);
}
