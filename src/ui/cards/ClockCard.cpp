#include "ClockCard.h"
#include <Arduino.h>
#include <time.h>
#include "../Theme.h"

static const char* WEEKDAYS[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char* MONTHS[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void ClockCard::init(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    _timeLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(_timeLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_timeLabel, "--:--");

    _dateLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_dateLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_dateLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_dateLabel, "---");
}

void ClockCard::update() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    if (!t || now < 1577836800LL) {
        lv_label_set_text(_timeLabel, "--:--");
        lv_label_set_text(_dateLabel, "---");
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
        lv_label_set_text(_timeLabel, buf);
        snprintf(buf, sizeof(buf), "%s %d %s", WEEKDAYS[t->tm_wday], t->tm_mday, MONTHS[t->tm_mon]);
        lv_label_set_text(_dateLabel, buf);
    }

    lv_obj_align(_timeLabel, LV_ALIGN_CENTER, 0, -45);
    lv_obj_align(_dateLabel, LV_ALIGN_CENTER, 0,  -3);
}

void ClockCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void ClockCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
