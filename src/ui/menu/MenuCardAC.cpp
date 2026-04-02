#include "MenuCardAC.h"
#include "../ScreenManager.h"
#include "../Theme.h"
#include "../fonts/fa_icons.h"

MenuCardAC::MenuCardAC(const char* name, const char* iconGlyph,
                       ACControlScreen& screen, ACState& ac)
    : _name(name), _iconGlyph(iconGlyph), _screen(screen), _ac(ac) {}

void MenuCardAC::init(lv_obj_t* container) {
    _iconLabel = lv_label_create(container);
    lv_obj_set_style_text_font(_iconLabel, &font_awesome_solid_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(_iconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_iconLabel, _iconGlyph);
    lv_obj_align(_iconLabel, LV_ALIGN_CENTER, 0, -40);

    _nameLabel = lv_label_create(container);
    lv_obj_set_style_text_font(_nameLabel, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(_nameLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_nameLabel, _name);
    lv_obj_align(_nameLabel, LV_ALIGN_CENTER, 0, +5);

    _stateLabel = lv_label_create(container);
    lv_obj_set_style_text_font(_stateLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_stateLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_stateLabel, "---");
    lv_obj_align(_stateLabel, LV_ALIGN_CENTER, 0, +40);
}

void MenuCardAC::update() {
    if (!_ac.valid) {
        lv_label_set_text(_stateLabel, "---");
        return;
    }
    if (strcmp(_ac.mode, "off") == 0) {
        lv_label_set_text(_stateLabel, "off");
    } else {
        char buf[24];
        snprintf(buf, sizeof(buf), "%.0f\xC2\xB0  %s", _ac.target_temp, _ac.mode);
        lv_label_set_text(_stateLabel, buf);
    }
}

void MenuCardAC::onSelect() {
    _screen.setAC(&_ac);
    screenManager.push(&_screen);
}
