#include <time.h>
#include "MeshCoreCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

// Y offsets from center (card area: ~y=-80 to y=+15)
static constexpr int ROW_ICON    = -80;  // tower icon (stacked above name)
static constexpr int ROW_NAME    = -50;  // "GigiTower" label
static constexpr int ROW_STATS1  = -16;  // battery | uptime
static constexpr int ROW_STATUS  = +12;  // "Last updated Xh ago"

// X offsets for stats layout: bat icon | bat% | caret | diff | uptime icon | uptime val
static constexpr int COL_L_ICON   = -85;
static constexpr int COL_L_VAL    = -57;
static constexpr int COL_TREND    = -33;
static constexpr int COL_DIFF     = -7;
static constexpr int COL_R_ICON   = +30;
static constexpr int COL_R_VAL    = +70;

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
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    // Header: tower icon stacked above name
    _headerIcon = lv_label_create(_container);
    lv_obj_set_style_text_font(_headerIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_headerIcon, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_headerIcon, FA_TOWER_BROADCAST);

    _headerName = lv_label_create(_container);
    lv_obj_set_style_text_font(_headerName, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(_headerName, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_headerName, "GigiTower");

    // Status dot (colored circle, size set in update)
    _statusDot = lv_obj_create(_container);
    lv_obj_set_size(_statusDot, 8, 8);
    lv_obj_set_style_radius(_statusDot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(_statusDot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_statusDot, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_statusDot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(_statusDot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_statusDot, LV_OBJ_FLAG_SCROLLABLE);

    // Status: "Last updated Xh ago"
    _statusLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_statusLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_statusLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);

    // Battery
    _batIcon = lv_label_create(_container);
    lv_obj_set_style_text_font(_batIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_batIcon, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_batIcon, FA_BATTERY_HALF);

    _batLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_batLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_batLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);

    // Battery trend caret
    _batTrendIcon = lv_label_create(_container);
    lv_obj_set_style_text_font(_batTrendIcon, &font_awesome_solid_18, LV_PART_MAIN);

    // Battery diff ("+2.1%")
    _batDiffLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_batDiffLabel, &lv_font_montserrat_14, LV_PART_MAIN);

    // Uptime
    _uptimeIcon = lv_label_create(_container);
    lv_obj_set_style_text_font(_uptimeIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_uptimeIcon, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_uptimeIcon, FA_CLOCK);

    _uptimeLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_uptimeLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_uptimeLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
}

void MeshCoreCard::update() {
    MeshCoreState& m = appState.meshcore;

    // Header
    lv_obj_align(_headerIcon, LV_ALIGN_CENTER, 0, ROW_ICON);
    lv_obj_align(_headerName, LV_ALIGN_CENTER, 0, ROW_NAME);

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

    // Status text: center it, then position dot dynamically to its left
    char agoBuf[32];
    formatAgo(agoBuf, sizeof(agoBuf), m.valid ? m.lastUpdatedAt : 0);
    lv_label_set_text(_statusLabel, agoBuf);
    lv_obj_align(_statusLabel, LV_ALIGN_CENTER, 0, ROW_STATUS);
    lv_obj_update_layout(_container);
    lv_coord_t labelW = lv_obj_get_width(_statusLabel);
    lv_obj_align(_statusDot, LV_ALIGN_CENTER, -(labelW / 2) - 4 - 4, ROW_STATUS);

    // Battery
    if (m.valid) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d%%", m.batteryPct);
        lv_label_set_text(_batLabel, buf);
    } else {
        lv_label_set_text(_batLabel, "--");
    }
    lv_obj_align(_batIcon,  LV_ALIGN_CENTER, COL_L_ICON, ROW_STATS1);
    lv_obj_align(_batLabel, LV_ALIGN_CENTER, COL_L_VAL,  ROW_STATS1);

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
    lv_obj_align(_batTrendIcon, LV_ALIGN_CENTER, COL_TREND, ROW_STATS1);
    lv_obj_align(_batDiffLabel, LV_ALIGN_CENTER, COL_DIFF,  ROW_STATS1);

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
    lv_obj_align(_uptimeIcon,  LV_ALIGN_CENTER, COL_R_ICON,  ROW_STATS1);
    lv_obj_align(_uptimeLabel, LV_ALIGN_CENTER, COL_R_VAL,   ROW_STATS1);
}

void MeshCoreCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void MeshCoreCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
