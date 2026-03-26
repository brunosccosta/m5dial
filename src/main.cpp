#include <M5Dial.h>
#include <lvgl.h>
#include "Input.h"
#include "CarouselMenu.h"

Input input;
MenuItem menuItems[] = {
    {"Lamps"},
    {"Air Conditioner"},
    {"Heater"},
    {"Settings"}
};
CarouselMenu menu(menuItems, 4);

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    M5Dial.Display.startWrite();
    M5Dial.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5Dial.Display.pushPixels((uint16_t *)px_map, w * h);
    M5Dial.Display.endWrite();

    lv_display_flush_ready(disp);
}

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    M5Dial.Display.setBrightness(128);

    lv_init();

    lv_display_t *disp = lv_display_create(240, 240);
    lv_display_set_flush_cb(disp, my_disp_flush);

    static lv_color_t buf[240 * 24];
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    input.begin();
    menu.init();
}

void loop() {
    M5Dial.update();
    lv_timer_handler();
    delay(5);

    input.update();

    int delta = input.getEncoderDelta();
    if (delta != 0) {
        menu.scroll(delta);
    }
    if (input.wasButtonPressed()) {
        menu.select();
    }
}
