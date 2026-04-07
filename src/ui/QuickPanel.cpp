#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include "QuickPanel.h"
#include "Theme.h"
#include "fonts/fa_icons.h"
#include "../AppState.h"

static const char* TAG = "QUICKPANEL";

QuickPanel quickPanel;

// ---------------------------------------------------------------------------
// Anim callback — drives panel y position
// ---------------------------------------------------------------------------
void QuickPanel::slideAnimCb(void* var, int32_t val) {
    lv_obj_set_y((lv_obj_t*)var, val);
}

// ---------------------------------------------------------------------------
// Find My tap handler
// ---------------------------------------------------------------------------
void QuickPanel::onFindMyTap(lv_event_t* e) {
    QuickPanel* self = (QuickPanel*)lv_event_get_user_data(e);
    if (millis() - self->_shownAtMs < OPEN_DEBOUNCE_MS) return;
    self->_findMyAction.trigger();
}

// ---------------------------------------------------------------------------
// init — create all LVGL objects once
// ---------------------------------------------------------------------------
void QuickPanel::init() {
    if (_initialized) return;
    _initialized = true;

    lv_obj_t* layer = lv_layer_top();

    // Full-screen dark panel, starts off the top
    _panel = lv_obj_create(layer);
    lv_obj_set_size(_panel, 240, 240);
    lv_obj_set_pos(_panel, 0, -240);
    lv_obj_set_style_bg_color(_panel, lv_color_hex(0x0A0A0A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_panel, LV_OBJ_FLAG_HIDDEN);

    // --- Connection row (y≈55 from panel top) ---
    _wifiIcon = lv_label_create(_panel);
    lv_obj_set_style_text_font(_wifiIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_wifiIcon, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_label_set_text(_wifiIcon, FA_WIFI);
    lv_obj_set_pos(_wifiIcon, 50, 47);

    _haIcon = lv_label_create(_panel);
    lv_obj_set_style_text_font(_haIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_haIcon, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_label_set_text(_haIcon, FA_HOUSE);
    lv_obj_set_pos(_haIcon, 74, 47);

    _connLabel = lv_label_create(_panel);
    lv_obj_set_style_text_font(_connLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_connLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_connLabel, "Connecting...");
    lv_obj_set_pos(_connLabel, 98, 49);

    // --- Indoor temps row (y≈105 from panel top) ---
    // Balcony
    lv_obj_t* iconBalcony = lv_label_create(_panel);
    lv_obj_set_style_text_font(iconBalcony, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(iconBalcony, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(iconBalcony, FA_SUN);
    lv_obj_set_pos(iconBalcony, 37, 100);

    _tempBalcony = lv_label_create(_panel);
    lv_obj_set_style_text_font(_tempBalcony, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tempBalcony, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_tempBalcony, "--°");
    lv_obj_set_pos(_tempBalcony, 60, 102);

    // Bedroom
    lv_obj_t* iconBedroom = lv_label_create(_panel);
    lv_obj_set_style_text_font(iconBedroom, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(iconBedroom, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(iconBedroom, FA_BED);
    lv_obj_set_pos(iconBedroom, 99, 100);

    _tempBedroom = lv_label_create(_panel);
    lv_obj_set_style_text_font(_tempBedroom, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tempBedroom, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_tempBedroom, "--°");
    lv_obj_set_pos(_tempBedroom, 125, 102);

    // Bathroom
    lv_obj_t* iconBathroom = lv_label_create(_panel);
    lv_obj_set_style_text_font(iconBathroom, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(iconBathroom, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(iconBathroom, FA_SHOWER);
    lv_obj_set_pos(iconBathroom, 166, 100);

    _tempBathroom = lv_label_create(_panel);
    lv_obj_set_style_text_font(_tempBathroom, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tempBathroom, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_tempBathroom, "--°");
    lv_obj_set_pos(_tempBathroom, 188, 102);

    // --- Find My button (y≈155 from panel top) ---
    _findMyBtn = lv_obj_create(_panel);
    lv_obj_set_size(_findMyBtn, 120, 36);
    lv_obj_set_pos(_findMyBtn, 60, 154);
    lv_obj_set_style_bg_color(_findMyBtn, lv_color_hex(Theme::SURFACE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_findMyBtn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_findMyBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(_findMyBtn, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_findMyBtn, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_findMyBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_findMyBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_findMyBtn, onFindMyTap, LV_EVENT_CLICKED, this);

    lv_obj_t* btnIcon = lv_label_create(_findMyBtn);
    lv_obj_set_style_text_font(btnIcon, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(btnIcon, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(btnIcon, FA_MOBILE);
    lv_obj_align(btnIcon, LV_ALIGN_CENTER, -42, 0);

    lv_obj_t* btnLabel = lv_label_create(_findMyBtn);
    lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(btnLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(btnLabel, "Find iPhone");
    lv_obj_align(btnLabel, LV_ALIGN_CENTER, 11, 0);

    ESP_LOGI(TAG, "init complete");
}

// ---------------------------------------------------------------------------
// show
// ---------------------------------------------------------------------------
void QuickPanel::show() {
    if (_visible || _animating) return;
    _visible   = true;
    _animating = true;
    _shownAtMs = millis();

    lv_obj_remove_flag(_panel, LV_OBJ_FLAG_HIDDEN);
    updateConnection();
    updateTemps();

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _panel);
    lv_anim_set_exec_cb(&a, slideAnimCb);
    lv_anim_set_values(&a, -240, 0);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, [](lv_anim_t*) {
        quickPanel._animating = false;
    });
    lv_anim_start(&a);

    ESP_LOGI(TAG, "show");
}

// ---------------------------------------------------------------------------
// dismiss
// ---------------------------------------------------------------------------
void QuickPanel::dismiss() {
    if (!_visible || _animating) return;
    _visible   = false;
    _animating = true;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _panel);
    lv_anim_set_exec_cb(&a, slideAnimCb);
    lv_anim_set_values(&a, 0, -240);
    lv_anim_set_duration(&a, 150);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_ready_cb(&a, [](lv_anim_t* a) {
        lv_obj_add_flag((lv_obj_t*)a->var, LV_OBJ_FLAG_HIDDEN);
        quickPanel._animating = false;
    });
    lv_anim_start(&a);

    ESP_LOGI(TAG, "dismiss");
}

// ---------------------------------------------------------------------------
// update — called every loop
// ---------------------------------------------------------------------------
void QuickPanel::update() {
    if (!_visible) return;
    updateConnection();
    updateTemps();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void QuickPanel::onEncoder(int /*delta*/) {}

void QuickPanel::onButton() {
    dismiss();
}

void QuickPanel::onTouch() {}

void QuickPanel::onSwipe(int dir) {
    if (dir < 0) dismiss(); // swipe up
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void QuickPanel::updateConnection() {
    switch (appState.connection) {
        case ConnectionState::WIFI_CONNECTING:
            lv_obj_set_style_text_color(_wifiIcon, lv_color_hex(0xFF3333), LV_PART_MAIN);
            lv_obj_set_style_text_color(_haIcon,   lv_color_hex(0x444444), LV_PART_MAIN);
            lv_label_set_text(_connLabel, "No WiFi");
            break;
        case ConnectionState::WIFI_CONNECTED:
        case ConnectionState::HA_CONNECTING:
            lv_obj_set_style_text_color(_wifiIcon, lv_color_hex(0x00CC44), LV_PART_MAIN);
            lv_obj_set_style_text_color(_haIcon,   lv_color_hex(0xCCAA00), LV_PART_MAIN);
            lv_label_set_text(_connLabel, "Connecting...");
            break;
        case ConnectionState::HA_READY: {
            lv_obj_set_style_text_color(_wifiIcon, lv_color_hex(0x00CC44), LV_PART_MAIN);
            lv_obj_set_style_text_color(_haIcon,   lv_color_hex(0x00CC44), LV_PART_MAIN);
            String ip = WiFi.localIP().toString();
            lv_label_set_text(_connLabel, ip.c_str());
            break;
        }
    }
}

void QuickPanel::updateTemps() {
    char buf[8];

    if (appState.sensors.outdoorTemp != 0.0f) {
        snprintf(buf, sizeof(buf), "%.0f°", appState.sensors.outdoorTemp);
    } else {
        snprintf(buf, sizeof(buf), "--°");
    }
    lv_label_set_text(_tempBalcony, buf);

    if (appState.sensors.bedroomTemp != 0.0f) {
        snprintf(buf, sizeof(buf), "%.0f°", appState.sensors.bedroomTemp);
    } else {
        snprintf(buf, sizeof(buf), "--°");
    }
    lv_label_set_text(_tempBedroom, buf);

    if (appState.sensors.bathroomTemp != 0.0f) {
        snprintf(buf, sizeof(buf), "%.0f°", appState.sensors.bathroomTemp);
    } else {
        snprintf(buf, sizeof(buf), "--°");
    }
    lv_label_set_text(_tempBathroom, buf);
}
