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

void RestScreen::setFooterTap(int side, std::function<void()> cb) {
    if (side < 0 || side > 1) return;
    _slots[side].onTap = cb;
}

void RestScreen::onFooterTap(lv_event_t* e) {
    FooterSlot* slot = static_cast<FooterSlot*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(lv_event_get_target(e))));
    slot->owner->_footerTapConsumed = true;
    slot->onTap();
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
    _cards[_cardCount++] = &_cardSpotify;

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

    // Device strip — two slots: 0=AC (left), 1=Heater (right)
    static constexpr int SLOT_X[2] = { -45, +45 };
    for (int s = 0; s < 2; s++) {
        _slots[s].owner = this;

        _slots[s].iconLabel = lv_label_create(_lvScreen);
        lv_obj_set_style_text_font(_slots[s].iconLabel, &font_awesome_solid_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(_slots[s].iconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
        lv_obj_align(_slots[s].iconLabel, LV_ALIGN_CENTER, SLOT_X[s], +52);

        _slots[s].stateLabel = lv_label_create(_lvScreen);
        lv_obj_set_style_text_font(_slots[s].stateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(_slots[s].stateLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
        lv_obj_align(_slots[s].stateLabel, LV_ALIGN_CENTER, SLOT_X[s], +74);

        if (_slots[s].onTap) {
            _slots[s].touchArea = lv_obj_create(_lvScreen);
            lv_obj_set_size(_slots[s].touchArea, 120, 80);
            lv_obj_set_pos(_slots[s].touchArea, s == 0 ? 0 : 120, 160);
            lv_obj_set_style_bg_opa(_slots[s].touchArea, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(_slots[s].touchArea, 0, LV_PART_MAIN);
            lv_obj_add_flag(_slots[s].touchArea, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_user_data(_slots[s].touchArea, &_slots[s]);
            lv_obj_add_event_cb(_slots[s].touchArea, onFooterTap, LV_EVENT_CLICKED, nullptr);
        }
    }

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
    // Skip invisible cards (e.g. Spotify when nothing is playing)
    for (int i = 0; i < _cardCount; i++) {
        _activeCard = (_activeCard + 1) % _cardCount;
        if (_cards[_activeCard]->isVisible()) break;
    }
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
    if (_footerTapConsumed) {
        _footerTapConsumed = false;
        return;
    }
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
    static constexpr int SLOT_X[2] = { -45, +45 };

    ACState* states[2] = { &appState.ac, &appState.heater };
    for (int s = 0; s < 2; s++) {
        ACState& state = *states[s];
        if (state.valid) {
            lv_label_set_text(_slots[s].iconLabel, modeIcon(state.mode));
            if (s == 1 && strcmp(state.mode, "off") == 0) {
                lv_label_set_text(_slots[s].stateLabel, "off");
            } else {
                snprintf(buf, sizeof(buf), "%s %.0f°", state.mode, state.target_temp);
                lv_label_set_text(_slots[s].stateLabel, buf);
            }
        } else {
            lv_label_set_text(_slots[s].iconLabel,  FA_POWER_OFF);
            lv_label_set_text(_slots[s].stateLabel, "---");
        }
        lv_obj_align(_slots[s].iconLabel,  LV_ALIGN_CENTER, SLOT_X[s], +52);
        lv_obj_align(_slots[s].stateLabel, LV_ALIGN_CENTER, SLOT_X[s], +74);
    }
}
