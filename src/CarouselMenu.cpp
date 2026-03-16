#include "CarouselMenu.h"

CarouselMenu::CarouselMenu(MenuItem* items, int count) 
    : menuItems(items), itemCount(count), currentIndex(0) {
}

void CarouselMenu::init() {
    // Create styles
    lv_style_init(&styleLarge);
    lv_style_set_text_font(&styleLarge, &lv_font_montserrat_32);
    lv_style_set_text_color(&styleLarge, lv_color_white());
    
    lv_style_init(&styleSmall);
    lv_style_set_text_font(&styleSmall, &lv_font_montserrat_14);
    lv_style_set_text_color(&styleSmall, lv_color_hex(0x888888)); // Gray
    
    // Create three labels
    // ...existing code...
}

void CarouselMenu::scroll(int delta) {
    if (delta == 0) return;
    
    // Update index (normalize delta to -1 or +1)
    int direction = (delta > 0) ? 1 : -1;
    currentIndex = wrapIndex(currentIndex + direction);
    
    updateDisplay();
}

void CarouselMenu::select() {
    // For now, just print - we'll add screen navigation later
    Serial.printf("Selected: %s\n", getCurrentLabel());
}

int CarouselMenu::getCurrentIndex() {
    return currentIndex;
}

const char* CarouselMenu::getCurrentLabel() {
    return menuItems[currentIndex].label;
}

void CarouselMenu::updateDisplay() {
    // Get the three items to display (with wrapping)
    int leftIdx = wrapIndex(currentIndex - 1);
    int centerIdx = currentIndex;
    int rightIdx = wrapIndex(currentIndex + 1);
    
    // Update label text
    lv_label_set_text(leftLabel, menuItems[leftIdx].label);
    lv_label_set_text(centerLabel, menuItems[centerIdx].label);
    lv_label_set_text(rightLabel, menuItems[rightIdx].label);
    
    // Re-center the labels (text length affects position)
    lv_obj_align(leftLabel, LV_ALIGN_CENTER, 0, -60);
    lv_obj_align(centerLabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(rightLabel, LV_ALIGN_CENTER, 0, 60);
}

int CarouselMenu::wrapIndex(int index) {
    // Circular array wrapping
    if (index < 0) {
        return itemCount - 1;
    } else if (index >= itemCount) {
        return 0;
    }
    return index;
}
