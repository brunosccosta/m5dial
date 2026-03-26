# Architecture

## Overview

Two independent lanes (backend, frontend) connected by a shared `AppState` singleton. Both run in the same Arduino loop — no RTOS tasks for now.

```
[HA WebSocket]  →  [AppState]  →  [LVGL Screens]
[LVGL Screens]  →  [AppState]  →  [HA WebSocket]
```

---

## AppState

Central shared state. A global singleton — typical and appropriate for single-core embedded.

```cpp
struct LampState {
    bool    on;
    uint8_t brightness; // 0–255
};

struct ACState {
    float  current_temp;
    float  target_temp;
    String mode; // "cool", "heat", "auto", "off"
};

struct AppState {
    LampState lamps[4];
    ACState   acs[2];
    float     room_temp;
};

extern AppState appState; // defined once in AppState.cpp
```

No business logic lives here. It's a plain data store.

---

## HAClient

Owns the WebSocket connection to Home Assistant. Non-blocking — `update()` is called every loop tick.

Responsibilities:
- Connect to WiFi and HA WebSocket API
- Authenticate with long-lived token
- Subscribe to entity state changes → write into `AppState`
- Expose command methods that send messages to HA

```cpp
class HAClient {
public:
    void begin(const char* ssid, const char* password,
               const char* ha_host, const char* ha_token);
    void update(); // call every loop(), non-blocking

    void setLamp(int id, bool on, uint8_t brightness);
    void setAC(int id, float target_temp, const char* mode);
};
```

**Library**: `Links2004/WebSockets` (most widely used ESP32 WebSocket lib).
**Protocol**: [Home Assistant WebSocket API](https://developers.home-assistant.io/docs/api/websocket)

---

## UI / Frontend

Pure C++ LVGL v9. No XML editor.

### Circular Menu
- Selected item: center of screen, large font
- Other items: arranged around the ring edge, small font, dimmed
- Dial rotation cycles items; button enters selected item's control screen

### Screen hierarchy
```
CircularMenu (main)
├── LampScreen
└── ACScreen
```

Each screen reads from `AppState` on load and on HA state updates. User interactions call `HAClient` methods.

---

## Loop structure

```cpp
void loop() {
    M5Dial.update();
    haClient.update();   // non-blocking HA/WiFi processing
    lv_timer_handler();  // LVGL rendering
    delay(5);

    // input handling → AppState / HAClient / UI
}
```

---

## Library choices

| Purpose | Library | Reason |
|---|---|---|
| Display + hardware | M5Unified + M5GFX + M5Dial | Official M5Stack libs |
| UI | LVGL v9 | Chosen UI framework |
| WebSocket | Links2004/WebSockets | Most widely used on ESP32 |

---

## Credentials

Hardcoded for now (toy project). In `credentials.h` (gitignored):

```cpp
#define WIFI_SSID     "your_ssid"
#define WIFI_PASSWORD "your_password"
#define HA_HOST       "homeassistant.local"
#define HA_TOKEN      "your_long_lived_token"
```

---

## Future considerations
- If HA comms cause UI jank, move `HAClient` to a FreeRTOS task with a mutex on `AppState`
- OTA updates: defer to final milestone
- Config persistence: SPIFFS or NVS for entity IDs and WiFi creds
