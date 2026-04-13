#pragma once
#include <lvgl.h>
#include "../ha/FindMyAction.h"
#include "../credentials.h"
#include "../AppState.h"
#include "ACControlScreen.h"

class QuickPanel {
public:
    void init(ACControlScreen& acCtrl, ACState& ac, ACState& heater);
    void show();
    void dismiss();
    void update();
    bool isVisible() const { return _visible; }

    // Input capture — all input is routed here while visible
    void onEncoder(int delta); // reserved, no-op for now
    void onButton();           // dismiss
    void onTouch();            // reserved, no-op for now
    void onSwipe(int dir);     // dir < 0 (up) → dismiss

private:
    static void slideAnimCb(void* var, int32_t val);
    static void onFindMyTap(lv_event_t* e);
    static void onACTap(lv_event_t* e);
    static void onHeaterTap(lv_event_t* e);

    void updateConnection();
    void updateAC();

    lv_obj_t* _panel        = nullptr;
    lv_obj_t* _wifiIcon     = nullptr;
    lv_obj_t* _haIcon       = nullptr;
    lv_obj_t* _connLabel    = nullptr;
    lv_obj_t* _acBtn         = nullptr;
    lv_obj_t* _acIcon        = nullptr;  // FA glyph (font_awesome_solid_18)
    lv_obj_t* _acTextLabel   = nullptr;  // mode/temp text (montserrat_14)
    lv_obj_t* _heaterBtn     = nullptr;
    lv_obj_t* _heaterIcon    = nullptr;
    lv_obj_t* _heaterTextLabel = nullptr;
    lv_obj_t* _findMyBtn    = nullptr;

    FindMyAction    _findMyAction { ICLOUD_ACCOUNT, ICLOUD_DEVICE_NAME };
    ACControlScreen* _acCtrl  = nullptr;
    ACState*         _ac      = nullptr;
    ACState*         _heater  = nullptr;

    static constexpr uint32_t OPEN_DEBOUNCE_MS = 300; // ignore taps this long after open

    bool     _visible     = false;
    bool     _initialized = false;
    uint32_t _shownAtMs   = 0;

public:
    bool _animating = false; // accessed by anim-ready lambdas in .cpp
};

extern QuickPanel quickPanel;
