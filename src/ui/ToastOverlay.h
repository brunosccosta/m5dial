#pragma once
#include <lvgl.h>

class ToastOverlay {
public:
    void init();
    void show(const char* icon, const char* message, uint32_t durationMs);
    void update();

private:
    static void fadeAnimCb(void* var, int32_t val);

    lv_obj_t* _pill      = nullptr;
    lv_obj_t* _iconLabel = nullptr;
    lv_obj_t* _msgLabel  = nullptr;

    uint32_t _hideAtMs    = 0;
    bool     _visible     = false;
    bool     _initialized = false;
};

extern ToastOverlay toast;
