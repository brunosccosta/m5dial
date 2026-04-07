#include <Arduino.h>
#include <time.h>
#include "FlightCard.h"
#include "../../flights.h"
#include "../fonts/fa_icons.h"
#include "../images/lvgl_images.h"
#include "../Theme.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

int FlightCard::daysUntil(int year, int month, int day) {
    time_t now = time(nullptr);
    struct tm* n = localtime(&now);
    n->tm_hour = 0; n->tm_min = 0; n->tm_sec = 0;
    time_t today = mktime(n);

    struct tm target = {};
    target.tm_year = year - 1900;
    target.tm_mon  = month - 1;
    target.tm_mday = day;
    target.tm_isdst = -1;
    return (int)((mktime(&target) - today) / 86400);
}

void FlightCard::countdownColor(int days, lv_color_t& out) {
    if (days <= 6)       out = lv_color_hex(COLOR_URGENT);
    else if (days <= 14) out = lv_color_hex(COLOR_WARN);
    else                 out = lv_color_hex(COLOR_NORMAL);
}

static const char* monthName(int m) {
    static const char* MONTHS[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    return (m >= 1 && m <= 12) ? MONTHS[m - 1] : "?";
}

static const char* dayOfWeek(int year, int month, int day) {
    static const char* DAYS[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;
    t.tm_isdst = -1;
    mktime(&t);
    return DAYS[t.tm_wday];
}

static const char* ordinalSuffix(int day) {
    if (day >= 11 && day <= 13) return "th";
    switch (day % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

void FlightCard::init(lv_obj_t* parent) {
    if (_initialized) return;
    _initialized = true;

    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // Header: plane icon + "Next Flight"
    lv_obj_t* headerIcon = lv_label_create(_container);
    lv_obj_set_style_text_font(headerIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(headerIcon, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_label_set_text(headerIcon, FA_PLANE_DEPARTURE);
    lv_obj_align(headerIcon, LV_ALIGN_CENTER, -42, -75);

    lv_obj_t* headerLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(headerLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(headerLabel, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_label_set_text(headerLabel, "Next Flight");
    lv_obj_align(headerLabel, LV_ALIGN_CENTER, +18, -75);

    // Flag image
    _flagImg = lv_image_create(_container);
    lv_obj_align(_flagImg, LV_ALIGN_CENTER, -40, -42);

    // Destination name
    _destLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_destLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_destLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_destLabel, LV_ALIGN_CENTER, +20, -42);

    // Countdown "45 days" / "Today!"
    _countLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_countLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(_countLabel, LV_ALIGN_CENTER, 0, -5);

    // Departure date "22 May"
    _dateLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_dateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_dateLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_align(_dateLabel, LV_ALIGN_CENTER, 0, -5);  // x set dynamically in update()

    // Hint "then Brazil in 48d"
    _hintLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_hintLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_hintLabel, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_obj_align(_hintLabel, LV_ALIGN_CENTER, 0, +20);

    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

// ---------------------------------------------------------------------------
// show / hide
// ---------------------------------------------------------------------------

void FlightCard::show() {
    lv_obj_remove_flag(_container, LV_OBJ_FLAG_HIDDEN);
    update();
}

void FlightCard::hide() {
    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

// ---------------------------------------------------------------------------
// isVisible
// ---------------------------------------------------------------------------

bool FlightCard::isVisible() const {
    for (int i = 0; i < FLIGHT_COUNT; i++) {
        if (daysUntil(FLIGHTS[i].year, FLIGHTS[i].month, FLIGHTS[i].day) >= 0)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void FlightCard::update() {
    // Find primary (next upcoming) and secondary flights
    int primary = -1, secondary = -1;
    for (int i = 0; i < FLIGHT_COUNT; i++) {
        int d = daysUntil(FLIGHTS[i].year, FLIGHTS[i].month, FLIGHTS[i].day);
        if (d >= 0) {
            if (primary == -1)        primary   = i;
            else if (secondary == -1) secondary = i;
        }
    }
    if (primary == -1) return;

    const FlightEntry& f = FLIGHTS[primary];
    int days = daysUntil(f.year, f.month, f.day);

    // Flag
    lv_image_set_src(_flagImg, f.flag);

    // Destination
    lv_label_set_text(_destLabel, f.destination);

    // Countdown
    lv_color_t col;
    countdownColor(days, col);
    lv_obj_set_style_text_color(_countLabel, col, LV_PART_MAIN);
    if (days == 0) {
        lv_label_set_text(_countLabel, "Today!");
    } else {
        char buf[24];
        snprintf(buf, sizeof(buf), "%d day%s", days, days == 1 ? "" : "s");
        lv_label_set_text(_countLabel, buf);
    }

    // Date — "/ Wed 22nd May" (smaller, bottom-aligned beside the countdown)
    char dateBuf[28];
    snprintf(dateBuf, sizeof(dateBuf), "/ %s %d%s %s",
             dayOfWeek(f.year, f.month, f.day),
             f.day, ordinalSuffix(f.day),
             monthName(f.month));
    lv_label_set_text(_dateLabel, dateBuf);

    // Hint line
    if (secondary != -1) {
        const FlightEntry& s = FLIGHTS[secondary];
        int sd = daysUntil(s.year, s.month, s.day);
        char hint[48];
        snprintf(hint, sizeof(hint), "then %s in %dd", s.destination, sd);
        lv_label_set_text(_hintLabel, hint);
        lv_obj_remove_flag(_hintLabel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_hintLabel, LV_OBJ_FLAG_HIDDEN);
    }

    // Re-center flag + destination as a group
    lv_obj_update_layout(_container);
    int flagW = lv_obj_get_width(_flagImg);
    int destW = lv_obj_get_width(_destLabel);
    int gap   = 8;
    int flagX = -(flagW + gap + destW) / 2 + flagW / 2;
    int destX = flagX + flagW / 2 + gap + destW / 2;
    lv_obj_align(_flagImg,   LV_ALIGN_CENTER, flagX, -42);
    lv_obj_align(_destLabel, LV_ALIGN_CENTER, destX, -42);

    // Re-center countdown + date side by side; date bottom-aligned to countdown bottom
    lv_obj_update_layout(_container);
    int countW = lv_obj_get_width(_countLabel);
    int countH = lv_obj_get_height(_countLabel);
    int dateW  = lv_obj_get_width(_dateLabel);
    int dateH  = lv_obj_get_height(_dateLabel);
    int rowGap = 8;
    int countX = -(countW + rowGap + dateW) / 2 + countW / 2;
    int dateX  = countX + countW / 2 + rowGap + dateW / 2;
    // countdown center at y=-5; date bottom matches countdown bottom
    int countY  = -5;
    int dateY   = countY + countH / 2 - dateH / 2;
    lv_obj_align(_countLabel, LV_ALIGN_CENTER, countX, countY);
    lv_obj_align(_dateLabel,  LV_ALIGN_CENTER, dateX,  dateY);
}
