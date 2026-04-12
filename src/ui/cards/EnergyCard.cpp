#include "EnergyCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

static lv_color_t scoreColor(float score) {
    if (score >= 80.0f) return lv_color_hex(Theme::AC_MODE_AUTO);
    if (score >= 50.0f) return lv_color_hex(Theme::TEMP_MID);
    return lv_color_hex(Theme::TEMP_WARM);
}

static lv_obj_t* makeRow(lv_obj_t* parent, lv_coord_t col_gap) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, col_gap, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

void EnergyCard::init(lv_obj_t* parent) {
    // Outer container — vertical flex column, centers all rows
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(_container, 30, LV_PART_MAIN); // shift content 15px up
    lv_obj_set_style_pad_row(_container, 8, LV_PART_MAIN);
    lv_obj_set_layout(_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    // Row 1: kWh number + "kWh\ntoday"
    lv_obj_t* kwhRow = makeRow(_container, 6);
    _kwhLabel = lv_label_create(kwhRow);
    lv_obj_set_style_text_font(_kwhLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_kwhLabel, "--");
    _kwhUnitLabel = lv_label_create(kwhRow);
    lv_obj_set_style_text_font(_kwhUnitLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_kwhUnitLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_kwhUnitLabel, "kWh\ntoday");

    // Row 2: bolt icon + watts + tariff
    lv_obj_t* wattsRow = makeRow(_container, 8);
    _boltIcon = lv_label_create(wattsRow);
    lv_obj_set_style_text_font(_boltIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_boltIcon, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_boltIcon, FA_BOLT);
    _wattsLabel = lv_label_create(wattsRow);
    lv_obj_set_style_text_font(_wattsLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_wattsLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_wattsLabel, "-- W");
    _tariffLabel = lv_label_create(wattsRow);
    lv_obj_set_style_text_font(_tariffLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tariffLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_tariffLabel, "-- ct");

    // Row 3: leaf icon + sustainability score
    lv_obj_t* scoreRow = makeRow(_container, 6);
    _leafIcon = lv_label_create(scoreRow);
    lv_obj_set_style_text_font(_leafIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_leafIcon, lv_color_hex(Theme::AC_MODE_AUTO), LV_PART_MAIN);
    lv_label_set_text(_leafIcon, FA_LEAF);
    _scoreLabel = lv_label_create(scoreRow);
    lv_obj_set_style_text_font(_scoreLabel, &lv_font_montserrat_14, LV_PART_MAIN);
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
