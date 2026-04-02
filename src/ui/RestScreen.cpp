#include <esp_log.h>
#include "RestScreen.h"
#include "../AppState.h"
#include "fonts/fa_icons.h"
#include "Theme.h"

static const char* TAG = "REST";

RestScreen restScreen;

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
    lv_obj_set_style_bg_color(_lvScreen, lv_color_hex(Theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(_lvScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Register cards — order defines display order. To add a card:
    //   1. Add a member instance above
    //   2. Add one line here
    _cards[_cardCount++] = &_cardClock;
    _cards[_cardCount++] = &_cardWeatherNow;
    _cards[_cardCount++] = &_cardIndoorTemps;
    _cards[_cardCount++] = &_cardForecast;

    // Init all cards; show first (or pinned), hide the rest
    _activeCard = (DEV_CARD_PIN >= 0 && DEV_CARD_PIN < _cardCount) ? DEV_CARD_PIN : 0;
    for (int i = 0; i < _cardCount; i++) {
        _cards[i]->init(_lvScreen);
        _cards[i]->hide();
    }
    _cards[_activeCard]->show();

    // Timer ring — thin arc at the outer edge, depletes clockwise from 12 o'clock
    _ring = lv_arc_create(_lvScreen);
    lv_obj_set_size(_ring, 240, 240);
    lv_obj_center(_ring);
    lv_arc_set_rotation(_ring, RING_START_ANGLE);
    lv_arc_set_bg_angles(_ring, 0, 360);
    lv_arc_set_range(_ring, 0, 360);
    lv_arc_set_value(_ring, 360);
    lv_obj_remove_style(_ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(_ring, RING_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_ring, RING_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(_ring, lv_color_hex(Theme::RING_BG),     LV_PART_MAIN);
    lv_obj_set_style_arc_color(_ring, lv_color_hex(Theme::RING_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(_ring, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(_ring, true, LV_PART_MAIN);

    // Device strip — AC (left)
    _acIconLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_acIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_acIconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_acIconLabel, LV_ALIGN_CENTER, -45, +42);

    _acStateLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_acStateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_acStateLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_align(_acStateLabel, LV_ALIGN_CENTER, -45, +60);

    // Device strip — Heater (right)
    _heaterIconLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_heaterIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_heaterIconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_heaterIconLabel, LV_ALIGN_CENTER, +45, +42);

    _heaterStateLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_heaterStateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_heaterStateLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_align(_heaterStateLabel, LV_ALIGN_CENTER, +45, +60);

    ESP_LOGI(TAG, "init complete, %d cards", _cardCount);
}

void RestScreen::show() {
    _lastAdvanceMs = millis();
    lv_arc_set_value(_ring, 360);
    _lastRingValue = 360;
    _cards[_activeCard]->update();
    updateDeviceStrip();
    lv_scr_load(_lvScreen);
    lv_refr_now(NULL);
}

void RestScreen::tick() {
    uint32_t now = millis();

    if (DEV_CARD_PIN < 0) {
        uint32_t elapsed = now - _lastAdvanceMs;
        if (elapsed >= CARD_INTERVAL_MS) {
            advanceCard();
            return;
        }
        int value = 360 - (int)(elapsed * 360 / CARD_INTERVAL_MS);
        if (value != _lastRingValue) {
            lv_arc_set_value(_ring, value);
            _lastRingValue = value;
        }
    }

    // Refresh active card every minute so the clock stays current
    if (now - _lastCardUpdateMs >= 60000) {
        _lastCardUpdateMs = now;
        _cards[_activeCard]->update();
        lv_refr_now(NULL);
    }
}

void RestScreen::advanceCard() {
    _cards[_activeCard]->hide();
    _activeCard = (_activeCard + 1) % _cardCount;
    _cards[_activeCard]->show();
    _cards[_activeCard]->update();
    _lastAdvanceMs = millis();
    lv_arc_set_value(_ring, 360);
    _lastRingValue = 360;
    lv_refr_now(NULL);
}

void RestScreen::onEncoder(int delta) {
    wake();
}

void RestScreen::onButton() {
    wake();
}

void RestScreen::onTouch() {
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
    _cards[_activeCard]->update();
    updateDeviceStrip();
}

void RestScreen::updateDeviceStrip() {
    char buf[24];

    ACState& ac = appState.ac;
    if (ac.valid) {
        lv_label_set_text(_acIconLabel, modeIcon(ac.mode));
        snprintf(buf, sizeof(buf), "%s %.0f°", ac.mode, ac.target_temp);
        lv_label_set_text(_acStateLabel, buf);
    } else {
        lv_label_set_text(_acIconLabel,  FA_POWER_OFF);
        lv_label_set_text(_acStateLabel, "---");
    }

    ACState& heater = appState.heater;
    if (heater.valid) {
        lv_label_set_text(_heaterIconLabel, modeIcon(heater.mode));
        if (strcmp(heater.mode, "off") == 0) {
            lv_label_set_text(_heaterStateLabel, "off");
        } else {
            snprintf(buf, sizeof(buf), "%s %.0f°", heater.mode, heater.target_temp);
            lv_label_set_text(_heaterStateLabel, buf);
        }
    } else {
        lv_label_set_text(_heaterIconLabel,  FA_POWER_OFF);
        lv_label_set_text(_heaterStateLabel, "---");
    }

    lv_obj_align(_acIconLabel,      LV_ALIGN_CENTER, -45, +42);
    lv_obj_align(_acStateLabel,     LV_ALIGN_CENTER, -45, +60);
    lv_obj_align(_heaterIconLabel,  LV_ALIGN_CENTER, +45, +42);
    lv_obj_align(_heaterStateLabel, LV_ALIGN_CENTER, +45, +60);
}
