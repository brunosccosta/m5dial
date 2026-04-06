#pragma once
#include <lvgl.h>

class RestCard {
public:
    virtual void init(lv_obj_t* parent) = 0;  // create LVGL objects once
    virtual void update()               = 0;  // refresh labels from AppState
    virtual void show()                 = 0;  // make objects visible
    virtual void hide()                 = 0;  // hide objects
    virtual bool isVisible() const      { return true; }
    virtual ~RestCard() = default;
};
