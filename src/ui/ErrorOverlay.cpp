#include <string.h>
#include <esp_log.h>
#include "ErrorOverlay.h"
#include "../AppState.h"
#include "Theme.h"

static const char* TAG = "OVERLAY";

static const struct { const char* key; const char* icon; const char* message; } CONFIGS[] = {
    { ErrorKey::WIFI,  LV_SYMBOL_WARNING, "No WiFi"        },
    { ErrorKey::HA_WS, LV_SYMBOL_WARNING, "HA Unreachable" },
};

ErrorOverlay errorOverlay;

// --- Animation callbacks ---

void ErrorOverlay::pulseAnimCb(void* var, int32_t val) {
    lv_obj_set_style_text_opa((lv_obj_t*)var, (lv_opa_t)val, LV_PART_MAIN);
}

void ErrorOverlay::fadeAnimCb(void* var, int32_t val) {
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)val, LV_PART_MAIN);
}

// --- Helpers ---

static void fadeIn(lv_obj_t* obj, uint32_t duration) {
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, ErrorOverlay::fadeAnimCb);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, duration);
    lv_anim_start(&a);
}

static void fadeOut(lv_obj_t* obj, uint32_t duration) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, ErrorOverlay::fadeAnimCb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_ready_cb(&a, [](lv_anim_t* a) {
        lv_obj_add_flag((lv_obj_t*)a->var, LV_OBJ_FLAG_HIDDEN);
    });
    lv_anim_start(&a);
}

// --- Public ---

void ErrorOverlay::init() {
    if (_initialized) return;
    _initialized = true;

    lv_obj_t* layer = lv_layer_top();

    // Full-screen black background
    _bg = lv_obj_create(layer);
    lv_obj_set_size(_bg, 240, 240);
    lv_obj_set_pos(_bg, 0, 0);
    lv_obj_set_style_bg_color(_bg, lv_color_hex(Theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_bg, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(_bg, 0, LV_PART_MAIN);
    lv_obj_add_flag(_bg, LV_OBJ_FLAG_HIDDEN);

    // Warning icon
    _icon = lv_label_create(_bg);
    lv_obj_set_style_text_font(_icon, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(_icon, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_icon, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_icon, 0, LV_PART_MAIN);
    lv_label_set_text(_icon, LV_SYMBOL_WARNING);
    lv_obj_align(_icon, LV_ALIGN_CENTER, 0, -20);

    // Message label
    _label = lv_label_create(_bg);
    lv_obj_set_style_text_font(_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_label, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_label, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_label, 0, LV_PART_MAIN);
    lv_obj_set_style_text_align(_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(_label, LV_ALIGN_CENTER, 0, 32);

    // Small dot for collapsed state
    _dot = lv_obj_create(layer);
    lv_obj_set_size(_dot, 10, 10);
    lv_obj_set_style_radius(_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_dot, lv_color_hex(Theme::ACCENT_ERROR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_dot, 0, LV_PART_MAIN);
    lv_obj_set_pos(_dot, 169, 19); // ~1 o'clock on the circular display (r≈106, inside r=120)
    lv_obj_add_flag(_dot, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "init complete");
}

void ErrorOverlay::update() {
    // Find first active error past its fireAfterMs
    const char* icon    = nullptr;
    const char* message = nullptr;

    uint32_t now = millis();
    for (int i = 0; i < AppState::MAX_ERRORS; i++) {
        ErrorEntry& e = appState.errors[i];
        if (!e.active || (now - e.registeredAt < e.fireAfterMs)) continue;

        for (auto& cfg : CONFIGS) {
            if (strncmp(cfg.key, e.key, sizeof(e.key)) == 0) {
                icon    = cfg.icon;
                message = cfg.message;
                break;
            }
        }
        if (!icon) { icon = LV_SYMBOL_WARNING; message = "Error"; }
        break;
    }

    if (!icon) {
        if (_state != State::HIDDEN) hideAll();
        return;
    }

    if (_state == State::HIDDEN) {
        showFull(icon, message);
    } else if (_state == State::FULL) {
        if (now - _shownAt > COLLAPSE_MS) collapseToDot();
    }
    // State::DOT — stays until error clears
}

// --- Private ---

void ErrorOverlay::showFull(const char* icon, const char* message) {
    _state   = State::FULL;
    _shownAt = millis();

    lv_label_set_text(_icon, icon);
    lv_label_set_text(_label, message);
    lv_obj_align(_icon,  LV_ALIGN_CENTER, 0, -20);
    lv_obj_align(_label, LV_ALIGN_CENTER, 0,  32);

    fadeIn(_bg, 300);

    // Pulse animation on icon opacity
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _icon);
    lv_anim_set_exec_cb(&a, pulseAnimCb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_40);
    lv_anim_set_duration(&a, 800);
    lv_anim_set_playback_duration(&a, 800);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);

    ESP_LOGW(TAG, "showing: %s", message);
}

void ErrorOverlay::collapseToDot() {
    _state = State::DOT;

    lv_anim_delete(_icon, pulseAnimCb); // stop pulse
    fadeOut(_bg, 400);
    fadeIn(_dot, 400);

    ESP_LOGD(TAG, "collapsed to dot");
}

void ErrorOverlay::hideAll() {
    _state = State::HIDDEN;

    lv_anim_delete(_icon, pulseAnimCb);

    if (!lv_obj_has_flag(_bg,  LV_OBJ_FLAG_HIDDEN)) fadeOut(_bg,  300);
    if (!lv_obj_has_flag(_dot, LV_OBJ_FLAG_HIDDEN)) fadeOut(_dot, 300);

    ESP_LOGI(TAG, "hidden — connection restored");
}
