#include "SolarCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"
#include "../CardLayout.h"

void SolarCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent);

    // Row 1: hero kWh + "kWh\ntoday"
    lv_obj_t* kwhRow = CardLayout::makeRow(_container);
    _kwhLabel = lv_label_create(kwhRow);
    lv_obj_set_style_text_font(_kwhLabel, CardLayout::fontHero(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_kwhLabel, "--");
    _kwhUnitLabel = lv_label_create(kwhRow);
    lv_obj_set_style_text_font(_kwhUnitLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhUnitLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_kwhUnitLabel, "Wh\ntoday");

    // Row 2: sun icon + live watts
    lv_obj_t* liveRow = CardLayout::makeRow(_container, CardLayout::PAD_ICON_TEXT + 2);
    _sunIcon = lv_label_create(liveRow);
    lv_obj_set_style_text_font(_sunIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_sunIcon, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_sunIcon, FA_SUN);
    _liveWLabel = lv_label_create(liveRow);
    lv_obj_set_style_text_font(_liveWLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_liveWLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_liveWLabel, "-- W");

    // Row 3: bolt icon + solar value EUR
    lv_obj_t* valueRow = CardLayout::makeRow(_container, CardLayout::PAD_ICON_TEXT + 2);
    _boltIcon = lv_label_create(valueRow);
    lv_obj_set_style_text_font(_boltIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_boltIcon, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_boltIcon, FA_BOLT);
    _valueEurLabel = lv_label_create(valueRow);
    lv_obj_set_style_text_font(_valueEurLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_valueEurLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_valueEurLabel, "-- ct solar");

    // Row 4: battery icon + savings EUR
    lv_obj_t* savingsRow = CardLayout::makeRow(_container, CardLayout::PAD_ICON_TEXT + 2);
    _battIcon = lv_label_create(savingsRow);
    lv_obj_set_style_text_font(_battIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_battIcon, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_battIcon, FA_BATTERY_HALF);
    _savingsEurLabel = lv_label_create(savingsRow);
    lv_obj_set_style_text_font(_savingsEurLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_savingsEurLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_savingsEurLabel, "-- ct saved");
}

void SolarCard::update() {
    SolarState& s = appState.solar;
    if (!s.valid) return;

    char buf[24];

    // Hero: today's energy in Wh (panels are small, kWh would show 0.x)
    snprintf(buf, sizeof(buf), "%.0f", s.energyTodayKwh * 1000.0f);
    lv_label_set_text(_kwhLabel, buf);

    // Live watts — yellow when producing, dim at night
    if (s.liveW >= 1.0f) {
        snprintf(buf, sizeof(buf), "%.0f W", s.liveW);
        lv_obj_set_style_text_color(_sunIcon,    lv_color_hex(Theme::TEMP_WARM), LV_PART_MAIN);
        lv_obj_set_style_text_color(_liveWLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    } else {
        lv_label_set_text(_liveWLabel, "0 W");
        lv_obj_set_style_text_color(_sunIcon,    lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
        lv_obj_set_style_text_color(_liveWLabel, lv_color_hex(Theme::TEXT_DIM),   LV_PART_MAIN);
        buf[0] = '\0';
    }
    if (buf[0]) lv_label_set_text(_liveWLabel, buf);

    // Solar value in cents (more readable than fractional EUR)
    int valueCt = (int)(s.valueTodayEur * 100.0f + 0.5f);
    snprintf(buf, sizeof(buf), "%d ct solar", valueCt);
    lv_label_set_text(_valueEurLabel, buf);

    // Battery savings in cents
    int savingsCt = (int)(s.savingsTodayEur * 100.0f + 0.5f);
    snprintf(buf, sizeof(buf), "%d ct saved", savingsCt);
    lv_label_set_text(_savingsEurLabel, buf);

    // Color savings row green when non-zero
    uint32_t savingsColor = savingsCt > 0 ? Theme::AC_MODE_AUTO : Theme::TEXT_FAINT;
    lv_obj_set_style_text_color(_battIcon,        lv_color_hex(savingsColor), LV_PART_MAIN);
    lv_obj_set_style_text_color(_savingsEurLabel, lv_color_hex(savingsColor), LV_PART_MAIN);
}

bool SolarCard::isVisible() const {
    return appState.solar.valid;
}

void SolarCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void SolarCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
