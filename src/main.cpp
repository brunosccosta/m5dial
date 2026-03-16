#include <M5Dial.h>
#include <lvgl.h>
#include "Input.h"

// ============================================================================
// UI INTEGRATION MODES
// ============================================================================
// Uncomment ONE of the following modes:

// MODE 1: Use hand-coded CarouselMenu (current temporary mode)
#define USE_MANUAL_UI
#include "CarouselMenu.h"

// MODE 2: Use LVGL Pro Editor generated UI (after exporting from editor)
// #define USE_GENERATED_UI
// #include "ui_generated/m5dial_ui.h"

// ============================================================================

#ifdef USE_MANUAL_UI
// Temporary: Manual UI with CarouselMenu
Input input;
MenuItem menuItems[] = {
    {"Lamps"},
    {"Air Conditioner"},
    {"Heater"},
    {"Settings"}
};
CarouselMenu menu(menuItems, 4);
#endif

#ifdef USE_GENERATED_UI
// Generated UI mode: Input handling will be integrated with LVGL events
Input input;
lv_obj_t* current_screen = NULL;
#endif

// 1. The Flush Callback: This tells LVGL how to draw on the M5 screen
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    M5Dial.Display.startWrite();
    M5Dial.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5Dial.Display.pushPixels((uint16_t *)&color_p->full, w * h);
    M5Dial.Display.endWrite();

    lv_disp_flush_ready(disp);
}

void setup() {
    M5Dial.begin();
    
    // Set brightness so you can see it!
    M5Dial.Display.setBrightness(128);

    // Initialize LVGL
    lv_init();

    // Setup Display Buffer (Use 1/10th of screen to save RAM)
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[240 * 24]; 
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 240 * 24);

    // Register the Display Driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize Input
    input.begin();

#ifdef USE_MANUAL_UI
    // Manual UI mode: Initialize CarouselMenu
    menu.init();
    Serial.println("M5Dial Smart Home Controller Ready! (Manual UI Mode)");
#endif

#ifdef USE_GENERATED_UILVGL task handler - must be called regularly
    delay(5);
    
    input.update();

#ifdef USE_MANUAL_UI
    // Manual UI mode: Handle input for CarouselMenu
    int delta = input.getEncoderDelta();
    if (delta != 0) {
        menu.scroll(delta);
    }
    
    if (input.wasButtonPressed()) {
        menu.select();
    }
#endif

#ifdef USE_GENERATED_UI
    // Generated UI mode: Input will be handled by LVGL events
    // You can add custom input handling here if needed
    // For example, encoder-based scrolling for lists or sliders
    
    int delta = input.getEncoderDelta();
    if (delta != 0) {
        // TODO: Send encoder events to LVGL
        // Example: Scroll focused object, adjust slider, etc.
        Serial.printf("Encoder delta: %d\n", delta);
    }
    
    if (input.wasButtonPressed()) {
        // TODO: Send click events to LVGL
        // Example: Click focused button, select item, etc.
        Serial.println("Button pressed");
    }
#endiferial.println("5. Rebuild and upload");
#endif
    
    Serial.println("\n=== M5Dial Smart Home Controller ===");
    Serial.println("Rotate dial to navigate");
    Serial.println("Press button to select");
    Serial.println("====================================\n");
}

void loop() {
    M5Dial.update();
    lv_timer_handler(); // This handles the animations
    delay(5);
    
    input.update();
    
    // Handle encoder rotation
    int delta = input.getEncoderDelta();
    if (delta != 0) {
        menu.scroll(delta);
    }
    
    // Handle button press
    if (input.wasButtonPressed()) {
        menu.select();
    }
}   