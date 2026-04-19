#include <Arduino.h>
#include <esp_log.h>
#include "ToastOverlay.h"
#include "Theme.h"
#include "fonts/fa_icons.h"

static const char* TAG = "TOAST";

ToastOverlay toast;

void ToastOverlay::fadeAnimCb(void* var, int32_t val) {
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)val, LV_PART_MAIN);
}

void ToastOverlay::init() {
    if (_initialized) return;
    _initialized = true;

    lv_obj_t* layer = lv_layer_top();

    _pill = lv_obj_create(layer);
    lv_obj_set_size(_pill, 200, 64);
    lv_obj_align(_pill, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(_pill, lv_color_hex(Theme::SURFACE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_pill, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_pill, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(_pill, 32, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(_pill, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(_pill, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(_pill, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(_pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_pill, LV_OBJ_FLAG_HIDDEN);

    _iconLabel = lv_label_create(_pill);
    lv_obj_set_style_text_font(_iconLabel, &font_awesome_solid_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_iconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_iconLabel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_iconLabel, 0, LV_PART_MAIN);

    _msgLabel = lv_label_create(_pill);
    lv_obj_set_style_text_font(_msgLabel, &font_montserrat_lat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_msgLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_msgLabel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_msgLabel, 0, LV_PART_MAIN);

    ESP_LOGI(TAG, "init complete");
}

void ToastOverlay::show(const char* icon, const char* message, uint32_t durationMs) {
    lv_label_set_text(_iconLabel, icon);
    lv_label_set_text(_msgLabel, message);
    lv_obj_align(_pill, LV_ALIGN_CENTER, 0, 0);

    lv_obj_remove_flag(_pill, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(_pill, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _pill);
    lv_anim_set_exec_cb(&a, fadeAnimCb);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, 200);
    lv_anim_start(&a);

    _hideAtMs = millis() + durationMs;
    _visible  = true;

    ESP_LOGI(TAG, "show: %s", message);
}

void ToastOverlay::update() {
    if (!_visible) return;
    if (millis() < _hideAtMs) return;

    _visible = false;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _pill);
    lv_anim_set_exec_cb(&a, fadeAnimCb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, 300);
    lv_anim_set_ready_cb(&a, [](lv_anim_t* a) {
        lv_obj_add_flag((lv_obj_t*)a->var, LV_OBJ_FLAG_HIDDEN);
    });
    lv_anim_start(&a);
}
