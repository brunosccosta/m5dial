#include <Arduino.h>
#include <esp_log.h>
#include "MenuScreen.h"
#include "../ScreenManager.h"
#include "../Theme.h"

static const char* TAG = "MENU";

// --- Card registration ---

void MenuScreen::addCard(MenuCard* card) {
    if (_cardCount < MAX_CARDS) _cards[_cardCount++] = card;
}

// --- Visibility + Arc indicator ---

void MenuScreen::rebuildVisible() {
    // Delete existing arcs
    for (int i = 0; i < MAX_CARDS; i++) {
        if (_arcs[i]) {
            lv_obj_delete(_arcs[i]);
            _arcs[i] = nullptr;
        }
    }

    // Recompute visible set
    _visibleCount = 0;
    for (int i = 0; i < _cardCount; i++) {
        if (_cards[i]->isVisible())
            _visibleIndices[_visibleCount++] = i;
    }

    // Clamp active card in case it fell outside the new visible set
    if (_activeCard >= _visibleCount) _activeCard = 0;

    if (_visibleCount == 0) return;

    // Rebuild arcs for visible count
    int segSize = (360 - _visibleCount * ARC_GAP_DEG) / _visibleCount;
    int segStep = 360 / _visibleCount;

    for (int i = 0; i < _visibleCount; i++) {
        _arcs[i] = lv_arc_create(_lvScreen);
        lv_obj_set_size(_arcs[i], 240, 240);
        lv_obj_center(_arcs[i]);
        lv_arc_set_rotation(_arcs[i], (RING_START_ANGLE + i * segStep) % 360);
        lv_arc_set_bg_angles(_arcs[i], 0, segSize);
        lv_arc_set_range(_arcs[i], 0, 1);
        lv_arc_set_value(_arcs[i], 0);
        lv_obj_remove_style(_arcs[i], NULL, LV_PART_KNOB);
        lv_obj_clear_flag(_arcs[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_width(_arcs[i], RING_WIDTH, LV_PART_MAIN);
        lv_obj_set_style_arc_width(_arcs[i], RING_WIDTH, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(_arcs[i], lv_color_hex(Theme::RING_BG),     LV_PART_MAIN);
        lv_obj_set_style_arc_color(_arcs[i], lv_color_hex(Theme::RING_ACTIVE), LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(_arcs[i], true, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(_arcs[i], true, LV_PART_MAIN);
    }
}

void MenuScreen::updateArcIndicator(int activeIdx) {
    for (int i = 0; i < _visibleCount; i++) {
        lv_arc_set_value(_arcs[i], i == activeIdx ? 1 : 0);
    }
}

// --- Lifecycle ---

void MenuScreen::init() {
    if (_initialized) return;
    _initialized = true;

    _lvScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_lvScreen, lv_color_hex(Theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(_lvScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_lvScreen, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    // Card containers — all at x=0 initially; off-screen ones moved in show()
    for (int i = 0; i < _cardCount; i++) {
        _containers[i] = lv_obj_create(_lvScreen);
        lv_obj_set_size(_containers[i], 240, 240);
        lv_obj_set_pos(_containers[i], 0, 0);
        lv_obj_set_style_bg_opa(_containers[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(_containers[i], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(_containers[i], 0, LV_PART_MAIN);
        lv_obj_clear_flag(_containers[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(_containers[i], LV_OBJ_FLAG_CLICKABLE);
        _cards[i]->init(_containers[i]);
    }

    // Arc segments — created after containers so they render on top
    rebuildVisible();

    ESP_LOGI(TAG, "init complete, %d cards (%d visible)", _cardCount, _visibleCount);
}

void MenuScreen::show() {
    _animating = false;

    // Recompute visible set — state may have changed since last visit
    rebuildVisible();

    // Reset positions: active visible card at 0, all others off-screen right
    int activeReal = _visibleCount > 0 ? _visibleIndices[_activeCard] : -1;
    for (int i = 0; i < _cardCount; i++) {
        lv_obj_set_pos(_containers[i], i == activeReal ? 0 : 240, 0);
    }

    if (_visibleCount > 0) {
        _cards[activeReal]->update();
        updateArcIndicator(_activeCard);
    }
    resetActivityTimer();

    lv_scr_load(_lvScreen);
    lv_refr_now(NULL);
}

// --- Input ---

void MenuScreen::onEncoder(int delta) {
    if (_animating || _visibleCount < 2) return;
    resetActivityTimer();

    int dir    = (delta > 0) ? -1 : 1;
    int newIdx = (_activeCard + dir + _visibleCount) % _visibleCount;
    slideTo(newIdx, dir);
}

void MenuScreen::onButton() {
    screenManager.pop();
}

void MenuScreen::onTouch() {
    if (_animating || _visibleCount == 0) return;
    resetActivityTimer();
    _cards[_visibleIndices[_activeCard]]->onSelect();
}

void MenuScreen::onSwipe(int dir) {
    onEncoder(dir);
}

void MenuScreen::refresh() {
    if (_visibleCount > 0)
        _cards[_visibleIndices[_activeCard]]->update();
}

void MenuScreen::tick() {
    if (millis() - _lastActivityMs >= INACTIVITY_MS) {
        ESP_LOGI(TAG, "inactivity timeout");
        resetActivityTimer(); // prevent re-trigger
        screenManager.pop();
    }
}

// --- Slide animation ---

static void slideExec(void* var, int32_t v) {
    lv_obj_set_x((lv_obj_t*)var, v);
}

void MenuScreen::onSlideComplete(lv_anim_t* a) {
    MenuScreen* self = static_cast<MenuScreen*>(lv_anim_get_user_data(a));
    self->_animating = false;
}

void MenuScreen::slideTo(int newIdx, int direction) {
    _animating = true;

    lv_obj_t* outCont = _containers[_visibleIndices[_activeCard]];
    lv_obj_t* inCont  = _containers[_visibleIndices[newIdx]];

    int inStart = direction > 0 ? 240 : -240;
    lv_obj_set_pos(inCont, inStart, 0);

    _cards[_visibleIndices[newIdx]]->update();
    updateArcIndicator(newIdx);

    // Outgoing container slides off
    lv_anim_t aOut;
    lv_anim_init(&aOut);
    lv_anim_set_var(&aOut, outCont);
    lv_anim_set_exec_cb(&aOut, slideExec);
    lv_anim_set_values(&aOut, 0, direction > 0 ? -240 : 240);
    lv_anim_set_duration(&aOut, SLIDE_ANIM_MS);
    lv_anim_start(&aOut);

    // Incoming container slides in; clears _animating when done
    lv_anim_t aIn;
    lv_anim_init(&aIn);
    lv_anim_set_var(&aIn, inCont);
    lv_anim_set_exec_cb(&aIn, slideExec);
    lv_anim_set_values(&aIn, inStart, 0);
    lv_anim_set_duration(&aIn, SLIDE_ANIM_MS);
    lv_anim_set_completed_cb(&aIn, onSlideComplete);
    lv_anim_set_user_data(&aIn, this);
    lv_anim_start(&aIn);

    _activeCard = newIdx;
}

void MenuScreen::resetActivityTimer() {
    _lastActivityMs = millis();
}
