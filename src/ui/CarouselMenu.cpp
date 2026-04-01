#include <esp_log.h>
#include <math.h>
#include <Arduino.h>
#include "CarouselMenu.h"
#include "ScreenManager.h"
#include "Theme.h"
#include "fonts/fa_icons.h"

static const char* TAG = "MENU";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
static constexpr float CAROUSEL_DEG_TO_RAD = M_PI / 180.0f;

CarouselMenu::CarouselMenu(MenuItem* items, int count)
    : _items(items), _count(count), _selectedIndex(0),
      _ringAngle(0.0f), _targetAngle(0.0f), _initialized(false),
      _lvScreen(nullptr), _centerIcon(nullptr), _centerLabel(nullptr) {
    for (int i = 0; i < MAX_ITEMS; i++) _ringIcons[i] = nullptr;
}

void CarouselMenu::init() {
    if (_initialized) return;
    _initialized = true;

    _lvScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_lvScreen, lv_color_hex(Theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);

    for (int i = 0; i < _count; i++) {
        lv_obj_t* icon = lv_label_create(_lvScreen);
        lv_obj_set_style_text_font(icon, &font_awesome_solid_32, LV_PART_MAIN);
        lv_obj_set_style_text_color(icon, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
        lv_obj_set_style_text_opa(icon, LV_OPA_50, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN);
        lv_label_set_text(icon, _items[i].icon);
        _ringIcons[i] = icon;
    }

    _centerIcon = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_centerIcon, &font_awesome_solid_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(_centerIcon, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_centerIcon, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_centerIcon, 0, LV_PART_MAIN);

    _centerLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_centerLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_centerLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_centerLabel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_centerLabel, 0, LV_PART_MAIN);
    lv_obj_set_style_text_align(_centerLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    updateCenter();
    updateRingPositions();

    ESP_LOGI(TAG, "init complete items=%d", _count);
}

void CarouselMenu::show() {
    _lastActivityMs = millis();
    lv_scr_load(_lvScreen);
    lv_refr_now(NULL);
}

void CarouselMenu::onEncoder(int delta) {
    _lastActivityMs = millis();
    scroll(delta);
}

void CarouselMenu::onButton() {
    _lastActivityMs = millis();
    select();
}

void CarouselMenu::refresh() {
    // Stub: will update icon opacity/state from AppState when integrated
    ESP_LOGD(TAG, "refresh");
}

void CarouselMenu::setOnSelect(std::function<void(int)> cb) {
    _onSelect = cb;
}

void CarouselMenu::enableInactivityTimer(uint32_t ms) {
    _inactivityMs = ms;
}

void CarouselMenu::tick() {
    if (_inactivityMs == 0) return;
    if (millis() - _lastActivityMs >= _inactivityMs) {
        _lastActivityMs = millis(); // reset to prevent repeated pops
        ESP_LOGI("MENU", "inactivity timeout, popping");
        screenManager.pop();
    }
}

int CarouselMenu::getCurrentIndex() const {
    return _selectedIndex;
}

void CarouselMenu::scroll(int delta) {
    if (delta == 0) return;

    int direction = (delta > 0) ? 1 : -1;
    _selectedIndex = ((_selectedIndex + direction) % _count + _count) % _count;

    float step = 360.0f / _count;
    _targetAngle -= direction * step;

    updateCenter();

    lv_anim_delete(this, CarouselMenu::animExecCb);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_exec_cb(&a, CarouselMenu::animExecCb);
    lv_anim_set_values(&a,
        (int32_t)(_ringAngle   * 100.0f),
        (int32_t)(_targetAngle * 100.0f));
    lv_anim_set_duration(&a, 250);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    ESP_LOGD(TAG, "scroll index=%d label=%s", _selectedIndex, _items[_selectedIndex].label);
}

void CarouselMenu::select() {
    ESP_LOGI(TAG, "select index=%d label=%s", _selectedIndex, _items[_selectedIndex].label);
    if (_onSelect) _onSelect(_selectedIndex);
}

void CarouselMenu::animExecCb(void* var, int32_t val) {
    CarouselMenu* self = static_cast<CarouselMenu*>(var);
    self->_ringAngle = val / 100.0f;
    self->updateRingPositions();
}

void CarouselMenu::updateRingPositions() {
    float step = 360.0f / _count;
    for (int i = 0; i < _count; i++) {
        float angle_deg = _ringAngle + i * step;
        float angle_rad = angle_deg * CAROUSEL_DEG_TO_RAD;
        int cx = 120 + (int)(RING_RADIUS * sinf(angle_rad));
        int cy = 120 - (int)(RING_RADIUS * cosf(angle_rad));
        lv_obj_set_pos(_ringIcons[i], cx - 16, cy - 16);
    }
}

void CarouselMenu::updateCenter() {
    lv_label_set_text(_centerIcon,  _items[_selectedIndex].icon);
    lv_label_set_text(_centerLabel, _items[_selectedIndex].label);
    lv_obj_align(_centerIcon,  LV_ALIGN_CENTER, 0, -20);
    lv_obj_align(_centerLabel, LV_ALIGN_CENTER, 0,  32);
    lv_refr_now(NULL);
}
