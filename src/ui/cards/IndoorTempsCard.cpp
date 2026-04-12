#include "IndoorTempsCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"
#include "../CardLayout.h"

// Fixed column widths — keeps all 3 rows identical in structure so columns align.
static constexpr int W_ROOM_ICON = 22; // FA glyph, center-aligned
static constexpr int W_TEMP      = 60; // "22.8°" at montserrat_24
static constexpr int W_DROPLET   = 20; // FA droplet glyph, center-aligned
static constexpr int W_HUMIDITY  = 34; // "100%" at montserrat_14

static constexpr int GAP_MIDDLE  = 20; // gap between the two column groups

static lv_obj_t* makeFixedIcon(lv_obj_t* parent, const char* glyph, uint32_t color, int w) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_set_width(lbl, w);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(color), LV_PART_MAIN);
    lv_label_set_text(lbl, glyph);
    return lbl;
}

static lv_obj_t* makeFixedLabel(lv_obj_t* parent, const lv_font_t* font,
                                uint32_t color, int w, const char* placeholder) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_set_width(lbl, w);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(color), LV_PART_MAIN);
    lv_label_set_text(lbl, placeholder);
    return lbl;
}

static void makeRoomRow(lv_obj_t* parent, const char* icon,
                        lv_obj_t** tempOut, lv_obj_t** humOut) {
    lv_obj_t* row = CardLayout::makeRow(parent, GAP_MIDDLE);

    // Group 1: room icon + temperature
    lv_obj_t* g1 = CardLayout::makeRow(row, CardLayout::PAD_ICON_TEXT);
    makeFixedIcon(g1, icon, Theme::TEXT_FAINT, W_ROOM_ICON);
    *tempOut = makeFixedLabel(g1, CardLayout::fontValue(), Theme::TEXT_PRIMARY, W_TEMP, "--.-°");

    // Group 2: droplet icon + humidity
    lv_obj_t* g2 = CardLayout::makeRow(row, CardLayout::PAD_ICON_TEXT);
    makeFixedIcon(g2, FA_DROPLET, Theme::ACCENT_RAIN, W_DROPLET);
    *humOut = makeFixedLabel(g2, CardLayout::fontDetail(), Theme::TEXT_DIM, W_HUMIDITY, "--%");
}

void IndoorTempsCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent, CardLayout::SHIFT_UP, CardLayout::PAD_ROW_LIST);

    makeRoomRow(_container, FA_SUN,    &_balconyTemp,  &_balconyHumidity);
    makeRoomRow(_container, FA_BED,    &_bedroomTemp,  &_bedroomHumidity);
    makeRoomRow(_container, FA_SHOWER, &_bathroomTemp, &_bathroomHumidity);
}

void IndoorTempsCard::updateRow(lv_obj_t* tempLabel, lv_obj_t* humLabel,
                                float temp, float humidity) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f°", temp);
    lv_label_set_text(tempLabel, buf);
    snprintf(buf, sizeof(buf), "%.0f%%", humidity);
    lv_label_set_text(humLabel, buf);
}

void IndoorTempsCard::update() {
    SensorState& s = appState.sensors;
    updateRow(_balconyTemp,  _balconyHumidity,  s.outdoorTemp,  s.outdoorHumidity);
    updateRow(_bedroomTemp,  _bedroomHumidity,  s.bedroomTemp,  s.bedroomHumidity);
    updateRow(_bathroomTemp, _bathroomHumidity, s.bathroomTemp, s.bathroomHumidity);
}

void IndoorTempsCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void IndoorTempsCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
