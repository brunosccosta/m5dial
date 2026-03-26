#pragma once
#include <lvgl.h>
#include "Screen.h"

class LampControlScreen : public Screen {
public:
    void setLampIndex(int idx);

    void init()               override;
    void show()               override;
    void onEncoder(int delta) override; // adjusts brightness
    void onButton()           override; // go back
    void refresh()            override;

private:
    void updateDisplay();

    int         _lampIdx    = 0;
    bool        _initialized = false;

    lv_obj_t*   _lvScreen;
    lv_obj_t*   _nameLabel;
    lv_obj_t*   _stateLabel;
    lv_obj_t*   _brightnessLabel;
};
