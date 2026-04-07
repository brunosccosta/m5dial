#pragma once
#include <lvgl.h>
#include "../ha/FindMyAction.h"
#include "../credentials.h"

class QuickPanel {
public:
    void init();
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

    void updateConnection();
    void updateTemps();

    lv_obj_t* _panel        = nullptr;
    lv_obj_t* _wifiIcon     = nullptr;
    lv_obj_t* _haIcon       = nullptr;
    lv_obj_t* _connLabel    = nullptr;
    lv_obj_t* _tempBalcony  = nullptr;
    lv_obj_t* _tempBedroom  = nullptr;
    lv_obj_t* _tempBathroom = nullptr;
    lv_obj_t* _findMyBtn    = nullptr;

    FindMyAction _findMyAction { ICLOUD_ACCOUNT, ICLOUD_DEVICE_NAME };

    static constexpr uint32_t OPEN_DEBOUNCE_MS = 300; // ignore taps this long after open

    bool     _visible     = false;
    bool     _initialized = false;
    uint32_t _shownAtMs   = 0;

public:
    bool _animating = false; // accessed by anim-ready lambdas in .cpp
};

extern QuickPanel quickPanel;
