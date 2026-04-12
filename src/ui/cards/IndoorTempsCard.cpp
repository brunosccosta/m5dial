#include "IndoorTempsCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"
#include "../CardLayout.h"

static lv_obj_t* makeRoomRow(lv_obj_t* parent, const char* icon,
                              lv_obj_t** tempOut, lv_obj_t** humOut) {
    lv_obj_t* row = CardLayout::makeRow(parent, 10);

    lv_obj_t* iconLbl = lv_label_create(row);
    lv_obj_set_style_text_font(iconLbl, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(iconLbl, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(iconLbl, icon);

    *tempOut = lv_label_create(row);
    lv_obj_set_style_text_font(*tempOut, CardLayout::fontValue(), LV_PART_MAIN);
    lv_obj_set_style_text_color(*tempOut, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(*tempOut, "--.-°");

    lv_obj_t* droplet = lv_label_create(row);
    lv_obj_set_style_text_font(droplet, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(droplet, lv_color_hex(Theme::ACCENT_RAIN), LV_PART_MAIN);
    lv_label_set_text(droplet, FA_DROPLET);

    *humOut = lv_label_create(row);
    lv_obj_set_style_text_font(*humOut, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(*humOut, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(*humOut, "--%");

    return row;
}

void IndoorTempsCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent);

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
