#include <esp_log.h>
#include <Arduino.h>
#include "ACControlScreen.h"
#include "../AppState.h"
#include "ScreenManager.h"
#include "../ha/HAClient.h"
#include "fonts/fa_icons.h"

static const char* TAG = "AC";

static constexpr const char* MODES[]   = { "heat", "cool", "auto", "fan_only", "dry", "off" };
static constexpr int         MODE_COUNT = 6;

struct ModeInfo { const char* icon; const char* label; lv_color_t color; };

static ModeInfo modeInfo(const char* mode) {
    if (strcmp(mode, "heat")     == 0) return { FA_FIRE,           "HEAT", lv_color_hex(0xFF4400) };
    if (strcmp(mode, "cool")     == 0) return { FA_SNOWFLAKE,      "COOL", lv_color_hex(0x0088FF) };
    if (strcmp(mode, "auto")     == 0) return { FA_ARROWS_ROTATE,  "AUTO", lv_color_hex(0x00BB44) };
    if (strcmp(mode, "fan_only") == 0) return { FA_FAN,            "FAN",  lv_color_hex(0xBBBBBB) };
    if (strcmp(mode, "dry")      == 0) return { FA_DROPLET,        "DRY",  lv_color_hex(0xFFAA00) };
    return                               { FA_POWER_OFF,       "OFF",  lv_color_hex(0x333333) };
}

// Current temp color follows temperature thresholds, not mode:
//   < 18°C  → blue   (cold)
//   18–21°C → yellow (transitional)
//   ≥ 21°C  → orange (warm)
static lv_color_t tempColor(float temp) {
    if (temp < 18.0f) return lv_color_hex(0x0088FF);
    if (temp < 21.0f) return lv_color_hex(0xFFCC00);
    return                   lv_color_hex(0xFF6600);
}

// --- Lifecycle ---

void ACControlScreen::setAC(ACState* ac) {
    _ac = ac;
}

void ACControlScreen::init() {
    if (_initialized) return;
    _initialized = true;

    _lvScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_lvScreen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(_lvScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Inner arc — current temp, smaller radius, thinner, dimmed
    _arcInner = lv_arc_create(_lvScreen);
    lv_obj_set_size(_arcInner, 190, 190);
    lv_obj_align(_arcInner, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_bg_start_angle(_arcInner, 185);
    lv_arc_set_bg_end_angle(_arcInner, 355);
    lv_arc_set_range(_arcInner, 10, 30);
    lv_arc_set_value(_arcInner, 10);
    lv_obj_set_style_arc_color(_arcInner, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arcInner, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arcInner, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(_arcInner, 160, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_arcInner, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(_arcInner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(_arcInner, LV_OBJ_FLAG_CLICKABLE);

    // Arc — target temp, outer track
    _arc = lv_arc_create(_lvScreen);
    lv_obj_set_size(_arc, 220, 220);
    lv_obj_align(_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_bg_start_angle(_arc, 185);
    lv_arc_set_bg_end_angle(_arc, 355);
    lv_arc_set_range(_arc, 10, 30);
    lv_arc_set_value(_arc, 21);
    lv_obj_set_style_arc_color(_arc, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(_arc, LV_OBJ_FLAG_CLICKABLE);

    // Arc range labels
    _arcMinLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_arcMinLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_arcMinLabel, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_label_set_text(_arcMinLabel, "10");
    lv_obj_align(_arcMinLabel, LV_ALIGN_CENTER, -100, 12);

    _arcMaxLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_arcMaxLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_arcMaxLabel, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_label_set_text(_arcMaxLabel, "30");
    lv_obj_align(_arcMaxLabel, LV_ALIGN_CENTER, 95, 12);

    // Current temp
    _currentTempLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_currentTempLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_currentTempLabel, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(_currentTempLabel, LV_ALIGN_CENTER, 0, -46);

    // Target temp — large, center
    _targetTempLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_targetTempLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(_targetTempLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_targetTempLabel, LV_ALIGN_CENTER, 0, -2);

    // Mode icon (FA font) + text (Montserrat), positioned side by side
    _modeIconLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_modeIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_modeIconLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_modeIconLabel, LV_ALIGN_CENTER, -20, 53);

    _modeTextLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_modeTextLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_modeTextLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_modeTextLabel, LV_ALIGN_CENTER, 8, 55); // will be overridden by align_to in updateDisplay

    // Go back
    _goBackLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_goBackLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_goBackLabel, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_label_set_text(_goBackLabel, LV_SYMBOL_LEFT);
    lv_obj_align(_goBackLabel, LV_ALIGN_CENTER, 0, 88);

    // Underline indicator — repositioned by updateUnderline()
    _underline = lv_obj_create(_lvScreen);
    lv_obj_set_style_bg_color(_underline, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_underline, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_underline, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(_underline, 0, LV_PART_MAIN);
    lv_obj_set_height(_underline, 2);
    lv_obj_clear_flag(_underline, LV_OBJ_FLAG_CLICKABLE);

    ESP_LOGI(TAG, "init complete");
}

void ACControlScreen::resetActivityTimer() {
    _lastActivityMs = millis();
}

void ACControlScreen::show() {
    // Start on mode when off — temp editing is blocked anyway
    ACState& ac = *_ac;
    _selected = (ac.valid && strcmp(ac.mode, "off") == 0) ? 1 : 0;
    _state    = State::HERO;
    resetActivityTimer();
    updateDisplay();
    lv_scr_load(_lvScreen);
    lv_obj_update_layout(_lvScreen); // force layout resolution before reading positions
    updateUnderline();
    lv_refr_now(NULL);
}

void ACControlScreen::tick() {
    if (millis() - _lastActivityMs >= INACTIVITY_TIMEOUT_MS) {
        ESP_LOGI(TAG, "inactivity timeout, popping");
        screenManager.pop();
    }
}

void ACControlScreen::onEncoder(int delta) {
    resetActivityTimer();
    ACState& ac = *_ac;

    switch (_state) {
        case State::HERO:
            _selected = (_selected + delta + 3) % 3;
            updateUnderline();
            break;

        case State::EDIT_TEMP:
            _pendingTemp -= delta * 0.5f;
            if (_pendingTemp < 10.0f) _pendingTemp = 10.0f;
            if (_pendingTemp > 30.0f) _pendingTemp = 30.0f;
            lv_arc_set_value(_arc, (int16_t)_pendingTemp);
            { char buf[8]; snprintf(buf, sizeof(buf), "%.1f°", _pendingTemp);
              lv_label_set_text(_targetTempLabel, buf); }
            break;

        case State::EDIT_MODE: {
            int count = ac.modeCount > 0 ? ac.modeCount : MODE_COUNT;
            _pendingModeIdx = (_pendingModeIdx + delta + count) % count;
            const char* modeName = ac.modeCount > 0 ? ac.availableModes[_pendingModeIdx] : MODES[_pendingModeIdx];
            ModeInfo info = modeInfo(modeName);
            lv_label_set_text(_modeIconLabel, info.icon);
            lv_label_set_text(_modeTextLabel, info.label);
            lv_obj_update_layout(_lvScreen);
            lv_obj_align_to(_modeTextLabel, _modeIconLabel, LV_ALIGN_OUT_RIGHT_MID, 6, 2);
            break;
        }
    }

    lv_refr_now(NULL);
}

void ACControlScreen::onButton() {
    resetActivityTimer();
    ACState& ac = *_ac;

    switch (_state) {
        case State::HERO:
            if (_selected == 2) { screenManager.pop(); return; }
            if (_selected == 0) {
                if (strcmp(ac.mode, "off") == 0) return; // can't edit temp when AC is off
                _pendingTemp = ac.target_temp;
                _state = State::EDIT_TEMP;
                lv_obj_set_style_bg_color(_underline, modeInfo(ac.mode).color, LV_PART_MAIN);
            } else {
                _pendingModeIdx = findModeIdx(ac.mode, ac);
                _state = State::EDIT_MODE;
                lv_obj_set_style_bg_color(_underline, modeInfo(ac.mode).color, LV_PART_MAIN);
            }
            break;

        case State::EDIT_TEMP:
            ac.target_temp = _pendingTemp;
            appState.dirty = true;
            haClient.sendACTemperature(ac.entity_id, _pendingTemp);
            _state = State::HERO;
            lv_obj_set_style_bg_color(_underline, lv_color_white(), LV_PART_MAIN);
            updateDisplay();
            break;

        case State::EDIT_MODE: {
            const char* newMode = ac.modeCount > 0 ? ac.availableModes[_pendingModeIdx] : MODES[_pendingModeIdx];
            strncpy(ac.mode, newMode, sizeof(ac.mode) - 1);
            ac.mode[sizeof(ac.mode) - 1] = '\0';
            appState.dirty = true;
            haClient.sendACMode(ac.entity_id, newMode);
            _state = State::HERO;
            lv_obj_set_style_bg_color(_underline, lv_color_white(), LV_PART_MAIN);
            updateDisplay();
            break;
        }
    }

    lv_refr_now(NULL);
}

void ACControlScreen::refresh() {
    updateDisplay();
}

// --- Display ---

int ACControlScreen::findModeIdx(const char* mode, const ACState& ac) {
    if (ac.modeCount > 0) {
        for (int i = 0; i < ac.modeCount; i++)
            if (strcmp(ac.availableModes[i], mode) == 0) return i;
        return 0;
    }
    for (int i = 0; i < MODE_COUNT; i++)
        if (strcmp(MODES[i], mode) == 0) return i;
    return 0;
}

void ACControlScreen::updateUnderline() {
    int32_t x, y, w;

    if (_selected == 1) {
        // Span underline across both icon and text labels
        x = lv_obj_get_x(_modeIconLabel);
        y = lv_obj_get_y(_modeIconLabel) + lv_obj_get_height(_modeIconLabel) + 3;
        w = (lv_obj_get_x(_modeTextLabel) + lv_obj_get_width(_modeTextLabel)) - x;
    } else {
        lv_obj_t* target = (_selected == 0) ? _targetTempLabel : _goBackLabel;
        x = lv_obj_get_x(target) + (_selected == 2 ? 2 : 0); // LV_SYMBOL_LEFT glyph has left padding, nudge right to visually center
        y = lv_obj_get_y(target) + lv_obj_get_height(target) + 3;
        w = lv_obj_get_width(target);
    }

    lv_obj_set_pos(_underline, x, y);
    lv_obj_set_width(_underline, w);
}

void ACControlScreen::updateDisplay() {
    ACState& ac = *_ac;

    if (ac.valid) {
        ModeInfo info = modeInfo(ac.mode);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°", ac.current_temp);
        lv_label_set_text(_currentTempLabel, buf);

        snprintf(buf, sizeof(buf), "%.1f°", ac.target_temp);
        lv_label_set_text(_targetTempLabel, buf);

        lv_label_set_text(_modeIconLabel, info.icon);
        lv_label_set_text(_modeTextLabel, info.label);

        lv_arc_set_value(_arc,      (int16_t)ac.target_temp);
        lv_arc_set_value(_arcInner, (int16_t)ac.current_temp);
        lv_obj_set_style_arc_color(_arc,      info.color,                  LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(_arcInner, tempColor(ac.current_temp), LV_PART_INDICATOR);
    } else {
        lv_label_set_text(_currentTempLabel, "--.-°");
        lv_label_set_text(_targetTempLabel, "--°");
        lv_label_set_text(_modeIconLabel, FA_POWER_OFF);
        lv_label_set_text(_modeTextLabel, "---");
        lv_obj_set_style_arc_color(_arc,      lv_color_hex(0x444444), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(_arcInner, lv_color_hex(0x444444), LV_PART_INDICATOR);
    }

    // Re-align after text update (text size may change)
    lv_obj_align(_currentTempLabel, LV_ALIGN_CENTER, 0, -46);
    lv_obj_align(_targetTempLabel,  LV_ALIGN_CENTER, 0, -2);
    lv_obj_align(_modeIconLabel, LV_ALIGN_CENTER, -20, 53);
    lv_obj_update_layout(_lvScreen); // needed before align_to so icon width is resolved
    lv_obj_align_to(_modeTextLabel, _modeIconLabel, LV_ALIGN_OUT_RIGHT_MID, 6, 2);
    lv_obj_align(_goBackLabel,      LV_ALIGN_CENTER, 0, 88);

    lv_refr_now(NULL);
}
