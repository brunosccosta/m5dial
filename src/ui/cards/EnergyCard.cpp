#include "EnergyCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

static lv_color_t scoreColor(float score) {
    if (score >= 80.0f) return lv_color_hex(Theme::AC_MODE_AUTO);
    if (score >= 50.0f) return lv_color_hex(Theme::TEMP_MID);
    return lv_color_hex(Theme::TEMP_WARM);
}

void EnergyCard::init(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    // Big kWh number — offset left to leave room for inline unit label
    _kwhLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_kwhLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_kwhLabel, "--");
    lv_obj_align(_kwhLabel, LV_ALIGN_CENTER, -20, -46);

    // "kWh\ntoday" — placed inline to the right of the number in update()
    _kwhUnitLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_kwhUnitLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhUnitLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_kwhUnitLabel, "kWh\ntoday");

    // Bolt icon
    _boltIcon = lv_label_create(_container);
    lv_obj_set_style_text_font(_boltIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_boltIcon, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_boltIcon, FA_BOLT);
    lv_obj_align(_boltIcon, LV_ALIGN_CENTER, -60, -2);

    // Current watts — 10px right of original position
    _wattsLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_wattsLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_wattsLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_wattsLabel, "-- W");
    lv_obj_align(_wattsLabel, LV_ALIGN_CENTER, -26, -2);

    // Tariff
    _tariffLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_tariffLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tariffLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_tariffLabel, "-- ct");
    lv_obj_align(_tariffLabel, LV_ALIGN_CENTER, +44, -2);

    // Leaf icon
    _leafIcon = lv_label_create(_container);
    lv_obj_set_style_text_font(_leafIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_leafIcon, lv_color_hex(Theme::AC_MODE_AUTO), LV_PART_MAIN);
    lv_label_set_text(_leafIcon, FA_LEAF);
    lv_obj_align(_leafIcon, LV_ALIGN_CENTER, -22, +20);

    // Score
    _scoreLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_scoreLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_scoreLabel, lv_color_hex(Theme::AC_MODE_AUTO), LV_PART_MAIN);
    lv_label_set_text(_scoreLabel, "--");
    lv_obj_align(_scoreLabel, LV_ALIGN_CENTER, +14, +20);
}

void EnergyCard::update() {
    EnergyState& e = appState.energy;
    if (!e.valid) return;

    // kWh — 1 decimal, number left of center, unit label to its right
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", e.dailyKwh);
    lv_label_set_text(_kwhLabel, buf);
    lv_obj_align(_kwhLabel, LV_ALIGN_CENTER, -20, -46);
    lv_obj_update_layout(_container);
    lv_obj_align_to(_kwhUnitLabel, _kwhLabel, LV_ALIGN_OUT_RIGHT_MID, 6, 0);

    // Watts
    if (e.currentW < 1000.0f)
        snprintf(buf, sizeof(buf), "%.0f W", e.currentW);
    else
        snprintf(buf, sizeof(buf), "%.1f kW", e.currentW / 1000.0f);
    lv_label_set_text(_wattsLabel, buf);

    // Tariff in cents
    snprintf(buf, sizeof(buf), "%.0f ct", e.tariff * 100.0f);
    lv_label_set_text(_tariffLabel, buf);

    // Score
    lv_color_t sc = scoreColor(e.sustainScore);
    lv_obj_set_style_text_color(_leafIcon,   sc, LV_PART_MAIN);
    lv_obj_set_style_text_color(_scoreLabel, sc, LV_PART_MAIN);
    snprintf(buf, sizeof(buf), "%.0f%%", e.sustainScore);
    lv_label_set_text(_scoreLabel, buf);

    lv_obj_align(_boltIcon,    LV_ALIGN_CENTER, -60, -2);
    lv_obj_align(_wattsLabel,  LV_ALIGN_CENTER, -26, -2);
    lv_obj_align(_tariffLabel, LV_ALIGN_CENTER, +44, -2);
    lv_obj_align(_leafIcon,    LV_ALIGN_CENTER, -22, +20);
    lv_obj_align(_scoreLabel,  LV_ALIGN_CENTER, +14, +20);
}

bool EnergyCard::isVisible() const {
    return appState.energy.valid;
}

void EnergyCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void EnergyCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
