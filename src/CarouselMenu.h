#pragma once
#include <lvgl.h>

struct MenuItem {
    const char* label;
    const char* icon; // LV_SYMBOL_* string
};

class CarouselMenu {
public:
    CarouselMenu(MenuItem* items, int count);
    void init();
    void refresh(); // call when AppState changes
    void scroll(int delta);
    void select();
    int  getCurrentIndex() const;

private:
    static void animExecCb(void* var, int32_t val);
    void updateRingPositions();
    void updateCenter();

    MenuItem*   _items;
    int         _count;
    int         _selectedIndex;
    float       _ringAngle;
    float       _targetAngle;

    static constexpr int MAX_ITEMS  = 8;
    static constexpr int RING_RADIUS = 85;

    lv_obj_t*   _ringIcons[MAX_ITEMS];
    lv_obj_t*   _centerIcon;
    lv_obj_t*   _centerLabel;
};
