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
#include "ui/ToastOverlay.h"
#include "ui/QuickPanel.h"
#include "ui/menu/MenuScreen.h"
#include "ui/menu/MenuCardAC.h"
#include "ui/menu/MenuCardFindMy.h"
#include "credentials.h"
#include "ui/fonts/fa_icons.h"
#include "SleepManager.h"
#include "Buzzer.h"
#include "RFIDReader.h"
#include "RFIDDispatcher.h"
#include "SpotifyHandler.h"
#include "HAHandler.h"
#include "SpotifyClient.h"

// --- AC control ---
ACControlScreen acControl;

// --- Main menu ---
MenuCardAC      menuCardAC("AC",     FA_WIND, acControl, appState.ac);
MenuCardAC      menuCardHeater("Heater", FA_FIRE, acControl, appState.heater);
MenuCardFindMy  menuCardFindMy(ICLOUD_ACCOUNT, ICLOUD_DEVICE_NAME);
MenuScreen      menuScreen;

Input input;
lv_indev_t* touchIndev = nullptr; // stored for lv_indev_reset() on swipe cancel

// Called when a gesture is classified as a swipe. Resets LVGL's "last pressed
// object" tracking so LV_EVENT_CLICKED never fires on whatever was under the
// finger. suppressNextRelease is a belt-and-suspenders fallback.
static bool suppressNextRelease = false;
static void cancelTouch() {
    suppressNextRelease = true;
    if (touchIndev) lv_indev_reset(touchIndev, nullptr);
}

void my_touch_read(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
    auto touch = M5Dial.Touch.getDetail();
    if (touch.isPressed()) {
        data->state   = LV_INDEV_STATE_PRESSED;
        data->point.x = touch.x;
        data->point.y = touch.y;
    } else {
        data->state   = LV_INDEV_STATE_RELEASED;
        if (suppressNextRelease) {
            suppressNextRelease = false;
            data->point.x = 0;
            data->point.y = 0;
        } else {
            data->point.x = touch.x;
            data->point.y = touch.y;
        }
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
    menuScreen.addCard(&menuCardFindMy);

    restScreen.setOnWake([]() {
        screenManager.push(&menuScreen);
    });

}

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, true);
    Serial.begin(115200);

    ESP_LOGI("BOOT", "M5Dial SmartHome starting");

    M5Dial.Display.setBrightness(128);
    sleepManager.begin(128);

    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return (uint32_t)millis(); });

    lv_display_t *disp = lv_display_create(240, 240);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_flush_cb(disp, my_disp_flush);

    static lv_color_t buf[240 * 24];
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    touchIndev = lv_indev_create();
    lv_indev_set_type(touchIndev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touchIndev, my_touch_read);

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
    spotifyClient.begin();
    rfidReader.begin();
    rfidDispatcher.registerHandler(&spotifyHandler);
    rfidDispatcher.registerHandler(&haHandler);
    rfidReader.onTag([](const char* uri) { rfidDispatcher.dispatch(uri); });
    haClient.begin(WIFI_SSID, WIFI_PASSWORD, HA_HOST, HA_PORT, HA_TOKEN);
    errorOverlay.init();
    toast.init();
    quickPanel.init(acControl, appState.ac, appState.heater);

    setupNavigation();
    screenManager.push(&restScreen);

    ESP_LOGI("BOOT", "setup complete");
}

void loop() {
    M5Dial.update();
    haClient.update();
    spotifyClient.update();

    // --- Input (before lv_timer_handler so swipe suppression takes effect) ---
    input.update();

    static constexpr int SWIPE_THRESHOLD         = 30; // px to classify any swipe
    static constexpr int PANEL_DISMISS_THRESHOLD = 15; // px upward movement to dismiss QuickPanel

    int delta = input.getEncoderDelta();
    if (delta != 0) {
        ESP_LOGD("INPUT", "encoder delta=%d", delta);
        sleepManager.onActivity();
        if (!sleepManager.isSleeping()) {
            buzzer.tick();
            if (quickPanel.isVisible()) quickPanel.onEncoder(-delta);
            else                        screenManager.onEncoder(-delta);
        }
    }
    if (input.wasButtonPressed()) {
        ESP_LOGD("INPUT", "button pressed");
        sleepManager.onActivity();
        if (!sleepManager.isSleeping()) {
            if (quickPanel.isVisible()) quickPanel.onButton();
            else                        screenManager.onButton();
        }
    }

    static int  touchStartX    = 0;
    static int  touchStartY    = 0;
    static bool gestureHandled = false; // prevents double-firing within one touch

    auto touch = M5Dial.Touch.getDetail();
    if (touch.wasPressed()) {
        sleepManager.onActivity();
        touchStartX    = touch.x;
        touchStartY    = touch.y;
        gestureHandled = false;
    } else if (!sleepManager.isSleeping() && touch.isPressed() && !gestureHandled && quickPanel.isVisible()) {
        // Early dismiss: fire as soon as the finger moves up past the threshold,
        // without waiting for release — panel is gone before the finger lifts.
        int dy = touch.y - touchStartY;
        if (dy < -PANEL_DISMISS_THRESHOLD) {
            gestureHandled = true;
            cancelTouch();
            quickPanel.onSwipe(-1);
        }
    } else if (!sleepManager.isSleeping() && touch.wasReleased() && !gestureHandled) {
        int dx = touch.x - touchStartX;
        int dy = touch.y - touchStartY;
        ESP_LOGI("TOUCH", "start=(%d,%d) end=(%d,%d) dx=%d dy=%d",
                 touchStartX, touchStartY, touch.x, touch.y, dx, dy);

        if (abs(dy) > abs(dx)) {
            // Vertical swipe
            if (dy > SWIPE_THRESHOLD) {
                cancelTouch();
                quickPanel.show();
            } else if (dy < -SWIPE_THRESHOLD) {
                cancelTouch();
                if (quickPanel.isVisible()) quickPanel.onSwipe(-1);
            } else {
                buzzer.tap();
                if (quickPanel.isVisible()) quickPanel.onTouch();
                else                        screenManager.onTouch();
            }
        } else {
            // Horizontal swipe
            if (dx < -SWIPE_THRESHOLD) {
                cancelTouch();
                if (quickPanel.isVisible()) quickPanel.onSwipe(+1);
                else                        screenManager.onSwipe(+1); // swipe left → next
            } else if (dx > SWIPE_THRESHOLD) {
                cancelTouch();
                if (quickPanel.isVisible()) quickPanel.onSwipe(-1);
                else                        screenManager.onSwipe(-1); // swipe right → prev
            } else {
                buzzer.tap();
                if (quickPanel.isVisible()) quickPanel.onTouch();
                else                        screenManager.onTouch();   // tap
            }
        }
    }

    // --- RFID ---
    rfidReader.update();

    // --- Render (touch suppression flag already set if needed) ---
    lv_timer_handler();
    delay(5);

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
    toast.update();
    quickPanel.update();

    screenManager.tick();
    sleepManager.tick();

    if (appState.dirty) {
        screenManager.refresh();
        appState.dirty = false;
    }
}
