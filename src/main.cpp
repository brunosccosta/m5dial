#include <M5Dial.h>
#include <lvgl.h>
#include <esp_log.h>
#include "input/Input.h"
#include "AppState.h"
#include "ha/HAClient.h"
#include "ui/ScreenManager.h"
#include "ui/ACControlScreen.h"
#include "ui/ConfirmScreen.h"
#include "ui/RestScreen.h"
#include "ui/ErrorOverlay.h"
#include "ui/menu/MenuScreen.h"
#include "ui/menu/MenuCardAC.h"
#include "credentials.h"
#include "ui/fonts/fa_icons.h"

// --- AC control ---
ACControlScreen acControl;

// --- Main menu ---
MenuCardAC    menuCardAC("AC",     FA_WIND, acControl, appState.ac);
MenuCardAC    menuCardHeater("Heater", FA_FIRE, acControl, appState.heater);
MenuScreen    menuScreen;

Input input;

void my_touch_read(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
    auto touch = M5Dial.Touch.getDetail();
    if (touch.isPressed()) {
        data->state   = LV_INDEV_STATE_PRESSED;
        data->point.x = touch.x;
        data->point.y = touch.y;
    } else {
        data->state   = LV_INDEV_STATE_RELEASED;
        data->point.x = touch.x;
        data->point.y = touch.y;
    }
}

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
    menuScreen.addCard(&menuCardAC);
    menuScreen.addCard(&menuCardHeater);

    restScreen.setOnWake([]() {
        screenManager.push(&menuScreen);
    });

    // Footer tap shortcuts — go directly to the control screen
    restScreen.setFooterTap(0, []() {
        acControl.setAC(&appState.ac);
        screenManager.push(&acControl);
    });
    restScreen.setFooterTap(1, []() {
        acControl.setAC(&appState.heater);
        screenManager.push(&acControl);
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
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_flush_cb(disp, my_disp_flush);

    static lv_color_t buf[240 * 24];
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read);

    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    if (M5Dial.Rtc.isEnabled() && M5Dial.Rtc.getVoltLow()) {
        auto dt = M5Dial.Rtc.getDateTime();
        if (dt.date.year >= 2020) {
            M5Dial.Rtc.setSystemTimeFromRtc();
            ESP_LOGI("BOOT", "system time seeded from RTC: %04d-%02d-%02d %02d:%02d",
                dt.date.year, dt.date.month, dt.date.date,
                dt.time.hours, dt.time.minutes);
        }
    }

    input.begin();
    haClient.begin(WIFI_SSID, WIFI_PASSWORD, HA_HOST, HA_PORT, HA_TOKEN);
    errorOverlay.init();

    setupNavigation();
    screenManager.push(&restScreen);

    ESP_LOGI("BOOT", "setup complete");
}

void loop() {
    M5Dial.update();
    haClient.update();
    lv_timer_handler();
    delay(5);

    input.update();

    int delta = input.getEncoderDelta();
    if (delta != 0) {
        ESP_LOGD("INPUT", "encoder delta=%d", delta);
        screenManager.onEncoder(-delta);
    }
    if (input.wasButtonPressed()) {
        ESP_LOGD("INPUT", "button pressed");
        screenManager.onButton();
    }

    static int touchStartX = 0;
    static constexpr int SWIPE_THRESHOLD = 30;

    auto touch = M5Dial.Touch.getDetail();
    if (touch.wasPressed()) {
        touchStartX = touch.x;
    } else if (touch.wasReleased()) {
        int dx = touch.x - touchStartX;
        ESP_LOGI("TOUCH", "start=%d end=%d dx=%d", touchStartX, touch.x, dx);
        M5Dial.Speaker.tone(300, 40);
        if (dx < -SWIPE_THRESHOLD) {
            screenManager.onSwipe(+1); // swipe left → next
        } else if (dx > SWIPE_THRESHOLD) {
            screenManager.onSwipe(-1); // swipe right → prev
        } else {
            screenManager.onTouch();  // tap
        }
    }

    static bool ntpSyncedToRtc = false;
    if (!ntpSyncedToRtc && M5Dial.Rtc.isEnabled()) {
        time_t t = time(nullptr);
        if (t > 1577836800LL) {
            M5Dial.Rtc.setDateTime(gmtime(&t));
            ntpSyncedToRtc = true;
            ESP_LOGI("NTP", "RTC synced from NTP");
        }
    }

    errorOverlay.update();

    screenManager.tick();

    if (appState.dirty) {
        screenManager.refresh();
        appState.dirty = false;
    }
}
