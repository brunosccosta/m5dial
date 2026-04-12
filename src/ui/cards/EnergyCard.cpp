#include "EnergyCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"
#include "../CardLayout.h"

static lv_color_t scoreColor(float score) {
    if (score >= 80.0f) return lv_color_hex(Theme::AC_MODE_AUTO);
    if (score >= 50.0f) return lv_color_hex(Theme::TEMP_MID);
    return lv_color_hex(Theme::TEMP_WARM);
}

void EnergyCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent);

    // Row 1: kWh number + "kWh\ntoday"
    lv_obj_t* kwhRow = CardLayout::makeRow(_container);
    _kwhLabel = lv_label_create(kwhRow);
    lv_obj_set_style_text_font(_kwhLabel, CardLayout::fontHero(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_kwhLabel, "--");
    _kwhUnitLabel = lv_label_create(kwhRow);
    lv_obj_set_style_text_font(_kwhUnitLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhUnitLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_kwhUnitLabel, "kWh\ntoday");

    // Row 2: bolt icon + watts + tariff
    lv_obj_t* wattsRow = CardLayout::makeRow(_container, CardLayout::PAD_ICON_TEXT + 2);
    _boltIcon = lv_label_create(wattsRow);
    lv_obj_set_style_text_font(_boltIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_boltIcon, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_boltIcon, FA_BOLT);
    _wattsLabel = lv_label_create(wattsRow);
    lv_obj_set_style_text_font(_wattsLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_wattsLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_wattsLabel, "-- W");
    _tariffLabel = lv_label_create(wattsRow);
    lv_obj_set_style_text_font(_tariffLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_tariffLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_tariffLabel, "-- ct");

    // Row 3: leaf icon + sustainability score
    lv_obj_t* scoreRow = CardLayout::makeRow(_container);
    _leafIcon = lv_label_create(scoreRow);
    lv_obj_set_style_text_font(_leafIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_leafIcon, lv_color_hex(Theme::AC_MODE_AUTO), LV_PART_MAIN);
    lv_label_set_text(_leafIcon, FA_LEAF);
    _scoreLabel = lv_label_create(scoreRow);
    lv_obj_set_style_text_font(_scoreLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_scoreLabel, lv_color_hex(Theme::AC_MODE_AUTO), LV_PART_MAIN);
    lv_label_set_text(_scoreLabel, "--");
}

void EnergyCard::update() {
    EnergyState& e = appState.energy;
    if (!e.valid) return;

    char buf[16];

    snprintf(buf, sizeof(buf), "%.1f", e.dailyKwh);
    lv_label_set_text(_kwhLabel, buf);

    if (e.currentW < 1000.0f)
        snprintf(buf, sizeof(buf), "%.0f W", e.currentW);
    else
        snprintf(buf, sizeof(buf), "%.1f kW", e.currentW / 1000.0f);
    lv_label_set_text(_wattsLabel, buf);

    snprintf(buf, sizeof(buf), "%.0f ct", e.tariff * 100.0f);
    lv_label_set_text(_tariffLabel, buf);

    lv_color_t sc = scoreColor(e.sustainScore);
    lv_obj_set_style_text_color(_leafIcon,   sc, LV_PART_MAIN);
    lv_obj_set_style_text_color(_scoreLabel, sc, LV_PART_MAIN);
    snprintf(buf, sizeof(buf), "%.0f%%", e.sustainScore);
    lv_label_set_text(_scoreLabel, buf);
}

bool EnergyCard::isVisible() const {
    return appState.energy.valid;
}

void EnergyCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void EnergyCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
