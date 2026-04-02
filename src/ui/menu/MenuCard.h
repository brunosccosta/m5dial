#pragma once
#include <lvgl.h>

// Base class for all main menu cards.
// MenuScreen creates a 240x240 container per card and passes it to init().
// onSelect() is responsible for any setup + screenManager.push().

class MenuCard {
public:
    virtual ~MenuCard() = default;
    virtual void        init(lv_obj_t* container) = 0;
    virtual void        update()                   = 0;
    virtual const char* label()                    = 0;
    virtual const char* icon()                     = 0;
    virtual void        onSelect()                 = 0;
};
