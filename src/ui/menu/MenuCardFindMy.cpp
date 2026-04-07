#include "MenuCardFindMy.h"
#include "../ScreenManager.h"
#include "../Theme.h"
#include "../fonts/fa_icons.h"

MenuCardFindMy::MenuCardFindMy(const char* account, const char* deviceName)
    : _action(account, deviceName) {}

void MenuCardFindMy::init(lv_obj_t* container) {
    _iconLabel = lv_label_create(container);
    lv_obj_set_style_text_font(_iconLabel, &font_awesome_solid_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(_iconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_iconLabel, FA_MOBILE);
    lv_obj_align(_iconLabel, LV_ALIGN_CENTER, 0, -40);

    _nameLabel = lv_label_create(container);
    lv_obj_set_style_text_font(_nameLabel, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(_nameLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_nameLabel, "Find iPhone");
    lv_obj_align(_nameLabel, LV_ALIGN_CENTER, 0, +5);
}

const char* MenuCardFindMy::icon() {
    return FA_MOBILE;
}

void MenuCardFindMy::onSelect() {
    _action.trigger();
    screenManager.pop();
}
