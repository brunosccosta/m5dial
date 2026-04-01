#include "WeatherDetailsCard.h"
#include "../../AppState.h"

void WeatherDetailsCard::init(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    _feelsLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_feelsLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_feelsLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(_feelsLabel, LV_ALIGN_CENTER, 0, -25);

    _outdoorLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_outdoorLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_outdoorLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(_outdoorLabel, LV_ALIGN_CENTER, 0, -5);

    _humidLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_humidLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_humidLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(_humidLabel, LV_ALIGN_CENTER, 0, +15);
}

void WeatherDetailsCard::update() {
    WeatherState& w = appState.weather;
    char buf[32];
    if (w.valid) {
        snprintf(buf, sizeof(buf), "feels like  %.0f°", w.feelsLike);
        lv_label_set_text(_feelsLabel, buf);
        snprintf(buf, sizeof(buf), "outside  %.1f°", w.outdoorTemp);
        lv_label_set_text(_outdoorLabel, buf);
        snprintf(buf, sizeof(buf), "humidity  %.0f%%", w.outdoorHumidity);
        lv_label_set_text(_humidLabel, buf);
    } else {
        lv_label_set_text(_feelsLabel,   "feels like  --°");
        lv_label_set_text(_outdoorLabel, "outside  --°");
        lv_label_set_text(_humidLabel,   "humidity  --%");
    }
    lv_obj_align(_feelsLabel,   LV_ALIGN_CENTER, 0, -25);
    lv_obj_align(_outdoorLabel, LV_ALIGN_CENTER, 0,  -5);
    lv_obj_align(_humidLabel,   LV_ALIGN_CENTER, 0, +15);
}

void WeatherDetailsCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); }
void WeatherDetailsCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
