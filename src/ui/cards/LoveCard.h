#pragma once
#include <lvgl.h>
#include "../RestCard.h"

class LoveCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;
    bool isVisible()      const override;

private:
    static void heartScaleAnimCb(void* var, int32_t val);
    static void msgTimerCb(lv_timer_t* timer);
    static void fadeOutDoneCb(lv_anim_t* a);
    static void fadeAnimCb(void* var, int32_t val);

    void startHeartAnim();
    void stopHeartAnim();
    void startMsgFadeOut();
    void updateIcon();  // shows heart or peach depending on active message

    lv_obj_t*  _container = nullptr;
    lv_obj_t*  _heart     = nullptr;
    lv_obj_t*  _peach     = nullptr;
    lv_obj_t*  _msgLabel  = nullptr;
    lv_timer_t* _msgTimer = nullptr;

    int  _msgIndex = 0;
    bool _initialized = false;

    static constexpr uint32_t MSG_INTERVAL_MS   = 8000;
    static constexpr uint32_t FADE_DURATION_MS  = 250;
    static constexpr uint32_t HEART_EXPAND_MS   = 500;
    static constexpr uint32_t HEART_DELAY_MS    = 3500;
    static constexpr int32_t  HEART_SCALE_NORM  = 256;
    static constexpr int32_t  HEART_SCALE_BIG   = 310;

    static const char* const MESSAGES[];
    static const int         MSG_COUNT;
};
