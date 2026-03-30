#pragma once
#include <functional>
#include <lvgl.h>
#include "Screen.h"

struct MenuItem {
    const char* label;
    const char* icon; // LV_SYMBOL_* string
};

class CarouselMenu : public Screen {
public:
    CarouselMenu(MenuItem* items, int count);

    void init()               override;
    void show()               override;
    void onEncoder(int delta) override;
    void onButton()           override;
    void refresh()            override;
    void tick()               override;

    void setOnSelect(std::function<void(int)> cb);
    void enableInactivityTimer(uint32_t ms); // pop after ms of no input
    int  getCurrentIndex() const;

private:
    static void animExecCb(void* var, int32_t val);
    void updateRingPositions();
    void updateCenter();
    void scroll(int delta);
    void select();

    MenuItem*   _items;
    int         _count;
    int         _selectedIndex;
    float       _ringAngle;
    float       _targetAngle;
    bool        _initialized;

    std::function<void(int)> _onSelect;

    uint32_t _inactivityMs      = 0;
    uint32_t _lastActivityMs    = 0;

    static constexpr int MAX_ITEMS   = 8;
    static constexpr int RING_RADIUS = 85;

    lv_obj_t*   _lvScreen;
    lv_obj_t*   _ringIcons[MAX_ITEMS];
    lv_obj_t*   _centerIcon;
    lv_obj_t*   _centerLabel;
};
