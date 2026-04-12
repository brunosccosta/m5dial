#include <time.h>
#include "MeshCoreCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"
#include "../CardLayout.h"

static void formatAgo(char* buf, int bufSize, time_t lastUpdatedAt) {
    time_t now = time(nullptr);
    if (now < 1577836800LL || lastUpdatedAt == 0) {
        snprintf(buf, bufSize, "Last updated --");
        return;
    }
    int diff = (int)(now - lastUpdatedAt);
    if      (diff < 60)    snprintf(buf, bufSize, "Last updated %ds ago", diff);
    else if (diff < 3600)  snprintf(buf, bufSize, "Last updated %dm ago", diff / 60);
    else if (diff < 86400) snprintf(buf, bufSize, "Last updated %dh ago", diff / 3600);
    else                   snprintf(buf, bufSize, "Last updated %dd ago", diff / 86400);
}

void MeshCoreCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent, 0, CardLayout::PAD_ROW_LIST);

    // Header: tower icon + node name
    lv_obj_t* hdrRow = CardLayout::makeRow(_container, CardLayout::PAD_ICON_TEXT);
    lv_obj_t* hdrIcon = lv_label_create(hdrRow);
    lv_obj_set_style_text_font(hdrIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(hdrIcon, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(hdrIcon, FA_TOWER_BROADCAST);
    lv_obj_t* hdrName = lv_label_create(hdrRow);
    lv_obj_set_style_text_font(hdrName, CardLayout::fontTitle(), LV_PART_MAIN);
    lv_obj_set_style_text_color(hdrName, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(hdrName, "GigiTower");

    // Stats: [bat icon | bat% | caret | diff]   [clock icon | uptime]
    lv_obj_t* statsRow = CardLayout::makeRow(_container, 24);

    lv_obj_t* batGroup = CardLayout::makeRow(statsRow, CardLayout::PAD_ICON_TEXT);
    lv_obj_t* batIcon = lv_label_create(batGroup);
    lv_obj_set_style_text_font(batIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(batIcon, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(batIcon, FA_BATTERY_HALF);
    _batLabel = lv_label_create(batGroup);
    lv_obj_set_style_text_font(_batLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_batLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    _batTrendIcon = lv_label_create(batGroup);
    lv_obj_set_style_text_font(_batTrendIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    _batDiffLabel = lv_label_create(batGroup);
    lv_obj_set_style_text_font(_batDiffLabel, CardLayout::fontDetail(), LV_PART_MAIN);

    lv_obj_t* uptimeGroup = CardLayout::makeRow(statsRow, CardLayout::PAD_ICON_TEXT);
    lv_obj_t* uptimeIcon = lv_label_create(uptimeGroup);
    lv_obj_set_style_text_font(uptimeIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(uptimeIcon, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(uptimeIcon, FA_CLOCK);
    _uptimeLabel = lv_label_create(uptimeGroup);
    lv_obj_set_style_text_font(_uptimeLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_uptimeLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);

    // Status: [dot] "Last updated Xh ago"
    lv_obj_t* statusRow = CardLayout::makeRow(_container, 6);
    _statusDot = lv_obj_create(statusRow);
    lv_obj_set_size(_statusDot, 8, 8);
    lv_obj_set_style_radius(_statusDot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(_statusDot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_statusDot, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_statusDot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(_statusDot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_statusDot, LV_OBJ_FLAG_SCROLLABLE);
    _statusLabel = lv_label_create(statusRow);
    lv_obj_set_style_text_font(_statusLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_statusLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
}

void MeshCoreCard::update() {
    MeshCoreState& m = appState.meshcore;

    // Status dot color based on age
    lv_color_t dotColor;
    if (!m.valid || m.lastUpdatedAt == 0) {
        dotColor = lv_color_hex(0x888888);
    } else {
        int diff = (int)(time(nullptr) - m.lastUpdatedAt);
        if      (diff < 6 * 3600)  dotColor = lv_color_hex(0x00CC44);
        else if (diff < 12 * 3600) dotColor = lv_color_hex(0xFFAA00);
        else                       dotColor = lv_color_hex(0xFF3333);
    }
    lv_obj_set_style_bg_color(_statusDot, dotColor, LV_PART_MAIN);

    // Status text
    char agoBuf[32];
    formatAgo(agoBuf, sizeof(agoBuf), m.valid ? m.lastUpdatedAt : 0);
    lv_label_set_text(_statusLabel, agoBuf);

    // Battery
    if (m.valid) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d%%", m.batteryPct);
        lv_label_set_text(_batLabel, buf);
    } else {
        lv_label_set_text(_batLabel, "--");
    }

    // Battery trend
    if (m.batteryDiffValid) {
        lv_color_t trendColor = m.batteryDiff >= 0
            ? lv_color_hex(0x00CC44)
            : lv_color_hex(0xFF3333);
        lv_label_set_text(_batTrendIcon, m.batteryDiff >= 0 ? FA_CARET_UP : FA_CARET_DOWN);
        lv_obj_set_style_text_color(_batTrendIcon, trendColor, LV_PART_MAIN);
        char diffBuf[12];
        snprintf(diffBuf, sizeof(diffBuf), "%+.1f%%", m.batteryDiff);
        lv_label_set_text(_batDiffLabel, diffBuf);
        lv_obj_set_style_text_color(_batDiffLabel, trendColor, LV_PART_MAIN);
    } else {
        lv_label_set_text(_batTrendIcon, "");
        lv_label_set_text(_batDiffLabel, "");
    }

    // Uptime
    if (m.valid) {
        char buf[12];
        uint32_t s  = m.uptimeSeconds;
        uint32_t d  = s / 86400;
        uint32_t h  = (s % 86400) / 3600;
        uint32_t mn = (s % 3600) / 60;
        if      (d >= 1) snprintf(buf, sizeof(buf), "%lud %luh", d, h);
        else if (h >= 1) snprintf(buf, sizeof(buf), "%luh %lum", h, mn);
        else             snprintf(buf, sizeof(buf), "%lum", mn);
        lv_label_set_text(_uptimeLabel, buf);
    } else {
        lv_label_set_text(_uptimeLabel, "--");
    }
}

void MeshCoreCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void MeshCoreCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
