#include <M5Dial.h>
#include <lvgl.h>
#include <esp_log.h>
#include "input/Input.h"
#include "AppState.h"
#include "ui/ScreenManager.h"
#include "ui/CarouselMenu.h"
#include "ui/LampControlScreen.h"

// --- Main menu ---
MenuItem mainItems[] = {
    {"Lamps",           LV_SYMBOL_EYE_OPEN},
    {"Air Conditioner", LV_SYMBOL_LOOP},
    {"Heater",          LV_SYMBOL_CHARGE},
    {"Settings",        LV_SYMBOL_SETTINGS},
};
CarouselMenu mainMenu(mainItems, 4);

// --- Lamp list (mirrors AppState.lamps + Go Back) ---
MenuItem lampItems[] = {
    {"Living Room", LV_SYMBOL_EYE_OPEN},
    {"Bedroom",     LV_SYMBOL_EYE_OPEN},
    {"Kitchen",     LV_SYMBOL_EYE_OPEN},
    {"Office",      LV_SYMBOL_EYE_OPEN},
    {"Go Back",     LV_SYMBOL_LEFT},
};
CarouselMenu lampMenu(lampItems, 5);

// --- Lamp control ---
LampControlScreen lampControl;

Input input;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    ESP_LOGV("DISPLAY", "flush (%d,%d)-(%d,%d)", area->x1, area->y1, area->x2, area->y2);
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5Dial.Display.startWrite();
    M5Dial.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5Dial.Display.pushPixels((uint16_t *)px_map, w * h);
    M5Dial.Display.endWrite();
    lv_display_flush_ready(disp);
}

void setupNavigation() {
    mainMenu.setOnSelect([](int idx) {
        switch (idx) {
            case 0: screenManager.push(&lampMenu); break; // Lamps
            default: ESP_LOGI("NAV", "no screen for index %d", idx); break;
        }
    });

    lampMenu.setOnSelect([](int idx) {
        if (idx == appState.lampCount) { // Go Back is last item
            screenManager.pop();
        } else {
            lampControl.setLampIndex(idx);
            screenManager.push(&lampControl);
        }
    });
}

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    Serial.begin(115200);

    ESP_LOGI("BOOT", "M5Dial SmartHome starting");

    M5Dial.Display.setBrightness(128);

    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return (uint32_t)millis(); });

    lv_display_t *disp = lv_display_create(240, 240);
    lv_display_set_flush_cb(disp, my_disp_flush);

    static lv_color_t buf[240 * 24];
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    input.begin();
    setupNavigation();
    screenManager.push(&mainMenu);

    ESP_LOGI("BOOT", "setup complete");
}

void loop() {
    M5Dial.update();
    lv_timer_handler();
    delay(5);

    input.update();

    int delta = input.getEncoderDelta();
    if (delta != 0) {
        ESP_LOGD("INPUT", "encoder delta=%d", delta);
        screenManager.onEncoder(delta);
    }
    if (input.wasButtonPressed()) {
        ESP_LOGD("INPUT", "button pressed");
        screenManager.onButton();
    }

    if (appState.dirty) {
        screenManager.refresh();
        appState.dirty = false;
    }
}
