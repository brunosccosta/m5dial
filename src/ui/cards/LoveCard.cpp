#include <Arduino.h>
#include "LoveCard.h"
#include "../fonts/fa_icons.h"
#include "../images/lvgl_images.h"
#include "../Theme.h"
#include "../CardLayout.h"
#include "../../AppState.h"

// ---------------------------------------------------------------------------
// Messages
// ---------------------------------------------------------------------------
const char* const LoveCard::MESSAGES[] = {
    "Я тебя люблю",
    "Моя любимая",
    "I love you",
    "Eu te amo",
    "Gostosa!",
};
const int LoveCard::MSG_COUNT = sizeof(MESSAGES) / sizeof(MESSAGES[0]);

// ---------------------------------------------------------------------------
// Anim callbacks
// ---------------------------------------------------------------------------
void LoveCard::heartScaleAnimCb(void* var, int32_t val) {
    lv_obj_set_style_transform_scale((lv_obj_t*)var, val, LV_PART_MAIN);
}

void LoveCard::fadeAnimCb(void* var, int32_t val) {
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)val, LV_PART_MAIN);
}

void LoveCard::fadeOutDoneCb(lv_anim_t* a) {
    lv_obj_t* label = (lv_obj_t*)a->var;
    LoveCard* self  = (LoveCard*)lv_obj_get_user_data(label);
    if (!self) return;

    self->_msgIndex = (self->_msgIndex + 1) % MSG_COUNT;
    lv_label_set_text(self->_msgLabel, MESSAGES[self->_msgIndex]);
    self->updateIcon();

    // Fade in
    lv_anim_t fa;
    lv_anim_init(&fa);
    lv_anim_set_var(&fa, label);
    lv_anim_set_exec_cb(&fa, fadeAnimCb);
    lv_anim_set_values(&fa, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&fa, FADE_DURATION_MS);
    lv_anim_start(&fa);
}

void LoveCard::msgTimerCb(lv_timer_t* timer) {
    LoveCard* self = (LoveCard*)lv_timer_get_user_data(timer);
    self->startMsgFadeOut();
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
void LoveCard::init(lv_obj_t* parent) {
    if (_initialized) return;
    _initialized = true;

    _container = CardLayout::makeContainer(parent, 0, CardLayout::PAD_ROW_LIST);

    // Icon slot — fixed size so flex layout doesn't shift when heart/peach swap
    lv_obj_t* iconSlot = lv_obj_create(_container);
    lv_obj_set_size(iconSlot, 40, 40);
    lv_obj_set_style_bg_opa(iconSlot, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(iconSlot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(iconSlot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(iconSlot, LV_OBJ_FLAG_SCROLLABLE);

    // Heart icon
    _heart = lv_label_create(iconSlot);
    lv_obj_set_style_text_font(_heart, &font_awesome_solid_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(_heart, lv_color_hex(0xFF3355), LV_PART_MAIN);
    lv_label_set_text(_heart, FA_HEART);
    lv_obj_align(_heart, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_transform_pivot_x(_heart, LV_PCT(50), LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(_heart, LV_PCT(50), LV_PART_MAIN);

    // Peach image (shown instead of heart on "Gostosa!")
    _peach = lv_image_create(iconSlot);
    lv_image_set_src(_peach, &emoji_peach);
    lv_obj_align(_peach, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(_peach, LV_OBJ_FLAG_HIDDEN);

    // Message label
    _msgLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_msgLabel, &font_montserrat_cyr_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_msgLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_msgLabel, MESSAGES[_msgIndex]);
    lv_obj_set_user_data(_msgLabel, this);

    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);

    // Message cycling timer (paused until show())
    _msgTimer = lv_timer_create(msgTimerCb, MSG_INTERVAL_MS, this);
    lv_timer_pause(_msgTimer);
}

// ---------------------------------------------------------------------------
// show / hide
// ---------------------------------------------------------------------------
void LoveCard::show() {
    lv_obj_remove_flag(_container, LV_OBJ_FLAG_HIDDEN);
    updateIcon();
    lv_timer_reset(_msgTimer);
    lv_timer_resume(_msgTimer);
}

void LoveCard::hide() {
    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    stopHeartAnim();
    lv_anim_delete(_msgLabel, fadeAnimCb);
    lv_timer_pause(_msgTimer);
}

// ---------------------------------------------------------------------------
// update / isVisible
// ---------------------------------------------------------------------------
void LoveCard::update() {}

bool LoveCard::isVisible() const {
    return appState.loveMode;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void LoveCard::updateIcon() {
    bool isGostosa = strcmp(MESSAGES[_msgIndex], "Gostosa!") == 0;
    if (isGostosa) {
        stopHeartAnim();
        lv_obj_add_flag(_heart, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(_peach, LV_OBJ_FLAG_HIDDEN);
    } else {
        stopHeartAnim();
        lv_obj_add_flag(_peach, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(_heart, LV_OBJ_FLAG_HIDDEN);
        startHeartAnim();
    }
}

void LoveCard::startHeartAnim() {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _heart);
    lv_anim_set_exec_cb(&a, heartScaleAnimCb);
    lv_anim_set_values(&a, HEART_SCALE_NORM, HEART_SCALE_BIG);
    lv_anim_set_duration(&a, HEART_EXPAND_MS);
    lv_anim_set_playback_duration(&a, HEART_EXPAND_MS);
    lv_anim_set_delay(&a, HEART_DELAY_MS);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

void LoveCard::stopHeartAnim() {
    lv_anim_delete(_heart, heartScaleAnimCb);
    lv_obj_set_style_transform_scale(_heart, HEART_SCALE_NORM, LV_PART_MAIN);
}

void LoveCard::startMsgFadeOut() {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _msgLabel);
    lv_anim_set_exec_cb(&a, fadeAnimCb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, FADE_DURATION_MS);
    lv_anim_set_ready_cb(&a, fadeOutDoneCb);
    lv_anim_start(&a);
}
