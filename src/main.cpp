#include <M5Unified.h>
#include <lvgl.h>

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(0); // Circular screen rotation

    // Initialize LVGL
    lv_init();
    
    // Minimal display buffer setup for LVGL
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t* buf = new lv_color_t[M5.Display.width() * 20]; 
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, M5.Display.width() * 20);

    // Simple label to test the screen
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "M5 Dial Ready!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void loop() {
    M5.update(); // Keep M5 hardware status updated
    lv_timer_handler(); // Update LVGL UI
    delay(5);
}