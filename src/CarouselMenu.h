#ifndef CAROUSEL_MENU_H
#define CAROUSEL_MENU_H

#include <Arduino.h>
#include <M5Unified.h>
#include <lvgl.h>

struct MenuItem {
    const char* label;
    // Future: add icon pointer
};

class CarouselMenu {
public:
    CarouselMenu(MenuItem* items, int itemCount);
    
    void init();
    void scroll(int delta);
    void select();
    int getCurrentIndex();
    const char* getCurrentLabel();
    
private:
    void updateDisplay();
    void animateTransition();
    int wrapIndex(int index);
    
    MenuItem* menuItems;
    int itemCount;
    int currentIndex;
    
    // LVGL objects
    lv_obj_t* leftLabel;
    lv_obj_t* centerLabel;
    lv_obj_t* rightLabel;
    
    // Styling
    lv_style_t styleLarge;
    lv_style_t styleSmall;
};

#endif
