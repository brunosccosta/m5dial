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

### Error registry

`AppState` owns a UI-agnostic error registry. Any subsystem can push or clear named errors:

```cpp
appState.setError(ErrorKey::WIFI,  5000); // show after 5s grace
appState.clearError(ErrorKey::WIFI);
```

Keys are defined as constants in `AppState.h` (`namespace ErrorKey`). The registry stores timing only — no icons or messages. `ErrorOverlay` owns the key → visual mapping.

`setError` is idempotent: calling it while a key is already active keeps the original timer running.

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

### Navigation model

`ScreenManager` owns a stack (max 4 deep). All input is routed through it to the active screen.

```
ScreenManager::push(screen) → screen->init() (once) → screen->show()
ScreenManager::pop()        → show previous screen
```

**Input contract:**
- Dial (encoder delta) → active screen's `onEncoder(delta)`
- Button press → active screen's `onButton()`
- In menus: button = select highlighted item
- In control screens: button = go back (`screenManager.pop()`)
- Dial in control screens = value adjustment (brightness, target temp)

### Screen hierarchy

```
CarouselMenu (main)              — Lamps / AC / Heater / Settings
└── CarouselMenu (lamp list)     — Living Room / Bedroom / … / ← Go Back
    └── LampControlScreen        — brightness dial, button = back
└── CarouselMenu (AC list)       — (future)
    └── ACControlScreen          — (future)
```

**Go Back** is always a ring item in sub-menus — never a gesture.

### CarouselMenu

Reusable circular navigation component. Implements `Screen`.
- Selected item shown large in center (icon + label)
- Other items as small dimmed icons around the ring at radius 85px
- Ring rotates via `lv_anim` (250ms ease-out) on encoder turn
- `setOnSelect(fn)` — caller wires navigation logic

### ErrorOverlay

Lives on `lv_layer_top()` — always above all screens. Three states:

- `HIDDEN` — nothing shown
- `FULL` — full-screen black bg, pulsing warning icon, message; collapses after 4s
- `DOT` — small red circle at ~1 o'clock position; stays until error clears

`update()` is called every loop. It scans `AppState.errors`, finds the first active entry past its `fireAfterMs`, looks up its icon/message via an internal config table, then drives the state machine. `appState.connection` is unchanged and available for other consumers (e.g. settings screen).

### Screen base class

```cpp
class Screen {
    virtual void init();             // called once on first push
    virtual void show();             // called on every push/pop
    virtual void onEncoder(int);
    virtual void onButton();
    virtual void refresh();          // called when AppState dirty
};
```

Each screen owns its own `lv_obj_t* _lvScreen` (LVGL screen object). `show()` calls `lv_scr_load`.

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
