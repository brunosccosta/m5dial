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
    const char* name;
    bool        on;
    uint8_t     brightness; // 0–255
};

struct ACState {
    const char* entity_id;
    const char* name;
    float       current_temp;
    float       target_temp;
    char        mode[12]; // "off", "cool", "heat", "auto", "fan_only", "dry"
    bool        valid;    // false until first HA update
};

struct WeatherState {
    char  condition[32];   // e.g. "sunny" — weather.buienradar state
    float temperature;     // weather.buienradar a.temperature
    float feelsLike;       // weather.buienradar a.apparent_temperature
    float outdoorTemp;     // sensor.atc_3294_temperature state
    float outdoorHumidity; // sensor.atc_3294_humidity state
    bool  valid;
};

struct AppState {
    LampState    lamps[4];
    int          lampCount;
    ACState      acs[AC_COUNT];
    WeatherState weather;
    bool         dirty;      // set by HA layer; cleared by UI after refresh
    ConnectionState connection;
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

### State machine

```
WIFI_CONNECTING → (WiFi up)  → WIFI_CONNECTED
WIFI_CONNECTED  → (WS open)  → HA_CONNECTING
HA_CONNECTING   → (auth ok)  → HA_READY
HA_READY        → (WS drop)  → HA_CONNECTING
any state       → (WiFi drop)→ WIFI_CONNECTING
```

State is written to `appState.connection` (readable by any UI component). Error overlay is driven via `appState.setError/clearError`.

### Auth flow

HA WebSocket protocol on connect:
1. Server sends `{"type":"auth_required"}`
2. Client sends `{"type":"auth","access_token":"TOKEN"}`
3. Server replies `{"type":"auth_ok"}` → `HA_READY` | `{"type":"auth_invalid"}` → log, stay in `HA_CONNECTING`

WS reconnects automatically (`setReconnectInterval(5000)`); HA resends `auth_required` each time so the flow repeats without extra logic.

### Heartbeat

`enableHeartbeat(15000, 3000, 2)` — WS ping every 15s, pong timeout 3s, 2 retries before disconnect.

### Entity subscription

After `auth_ok`, HAClient sends a single `subscribe_entities` message with all entity IDs from `devices.h`:

```json
{"id":1,"type":"subscribe_entities","entity_ids":["climate.forninho_room_temperature","climate.forninho_portatil"]}
```

**Why not `get_states`**: `get_states` returns all HA entities in one WS frame — easily 50–200KB. The Links2004 WS library drops the connection when the incoming frame exceeds its buffer. `subscribe_entities` with specific IDs returns only the requested entities.

HA responds with:
- A `result` confirmation (id match, success)
- An immediate `event.a` (added) with current state for all subscribed entities
- Subsequent `event.c` (changed) diffs — only the fields that changed

```json
// Initial: event.a
{"event":{"a":{"climate.x":{"s":"heat","a":{"current_temperature":23.0,"temperature":21.0}}}}}

// Change: event.c, "+" = updated fields only
{"event":{"c":{"climate.x":{"+":{"s":"off"}}}}}
```

`HAClient::updateACState()` handles both cases — `state` or `attrs` may be null on a diff (only changed fields are present).

### Sending commands

`HAClient` exposes two methods for AC control:

```cpp
haClient.sendACTemperature(entity_id, temp);  // climate.set_temperature
haClient.sendACMode(entity_id, mode);         // climate.set_hvac_mode
```

Payload format:

```json
{"id":2,"type":"call_service","domain":"climate","service":"set_temperature",
 "service_data":{"temperature":21.5},"target":{"entity_id":"climate.x"}}

{"id":3,"type":"call_service","domain":"climate","service":"set_hvac_mode",
 "service_data":{"hvac_mode":"heat"},"target":{"entity_id":"climate.x"}}
```

`_msgId` is incremented on each real send to keep message IDs unique across the session.

### Dry-run mode

`begin()` accepts an optional `dryRun` flag (default `false`). When enabled, send methods log the payload at INFO level instead of transmitting — useful for validating payloads without touching HA:

```cpp
haClient.begin(WIFI_SSID, WIFI_PASSWORD, HA_HOST, HA_PORT, HA_TOKEN, true);
```

**Libraries**: `Links2004/WebSockets`, `ArduinoJson@^7`

---

## Device configuration

Entity IDs and display names live in `src/devices.h` (gitignored). Struct definitions in `src/DeviceConfig.h` (committed). `src/devices.h.example` is committed as a template.

```cpp
// devices.h
constexpr DeviceEntry ACS[] = {
    { "climate.forninho_room_temperature", "Forninho"          },
    { "climate.forninho_portatil",         "Forninho Portátil" },
};
constexpr int AC_COUNT = sizeof(ACS) / sizeof(ACS[0]);
```

`AppState.acs[]` is sized by `AC_COUNT` at compile time. `AppState::initDevices()` (called at boot) copies entity_id and name pointers from `ACS[]` into each slot.

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
- In ACControlScreen: dial cycles selected property, button confirms/edits (see below)
- Any input on RestScreen → push main CarouselMenu
- Auto-return to RestScreen after 30s inactivity on main menu
- Auto-return to previous screen after 30s inactivity on any control screen

### Screen hierarchy

```
RestScreen (root)                — idle/screensaver, boots here
└── CarouselMenu (main)          — Lamps / AC / Heater
    └── CarouselMenu (lamp list) — Living Room / Bedroom / … / ← Go Back
        └── LampControlScreen   — brightness dial, button = back
    └── ACControlScreen         — direct push, no list (single device skips list)
```

**RestScreen** is the root of the stack — it is never popped. Any dial or button input pushes the main CarouselMenu. After 30s inactivity on the main menu it pops back to RestScreen. ACControlScreen already has its own 30s timer that pops to the previous screen.

**Go Back** is a selectable property in ACControlScreen (dial to it, button confirms) — no gestures.

### RestScreen

Idle/screensaver shown at boot and after 30s inactivity on the main menu. Displays at-a-glance home status — no interaction beyond "wake up".

**Layout** (240×240 circle):

```
         ☁  15°          ← current weather: condition icon + temp   (~y=50)
        Cloudy            ← condition text

       → 🌤  17°         ← 1h forecast: arrow + icon + temp         (~y=105)

         12.4°            ← outside temp, small grey                (~y=140)

   ❄ AUTO 21°  🔥 OFF    ← AC + heater status, side by side        (~y=175)
```

**Data sources:**

| Field | HA entity | Attribute |
|---|---|---|
| Current condition | `weather.*` | `state` (e.g. `"cloudy"`) |
| Current temp | `weather.*` | `temperature` attribute |
| 1h forecast | `weather.*` | `forecast` attribute (first entry) |
| Outside temp | `sensor.atc_3294_temperature` | `state` |
| AC status | `climate.forninho_room_temperature` | already in `appState.acs[0]` |
| Heater status | `climate.forninho_portatil` | already in `appState.acs[1]` |

**Weather icons:** FontAwesome Solid glyphs mapped to HA condition strings. Animated icons deferred — static FA icons first, animation as a later enhancement.

**HA weather forecast:** Buienradar does not include forecast in `subscribe_entities` — the payload only contains current conditions. Forecast requires a separate `weather.get_forecasts` service call (see below).

**Confirmed fields from `weather.buienradar` via `subscribe_entities`:**

```json
"s": "sunny",
"a": {
  "temperature": 3.7,
  "apparent_temperature": 0.9,
  "humidity": 79,
  "wind_speed": 10.8,
  "wind_bearing": 298,
  "pressure": 1024.8
}
```

Fields used by RestScreen: `s` (condition), `temperature` (current), `apparent_temperature` (feels like).

**Fetching hourly forecast:**

Forecast requires a one-shot `call_service` + response via `result`:
```json
{"id":N,"type":"call_service","domain":"weather","service":"get_forecasts",
 "service_data":{"type":"hourly"},"target":{"entity_id":"weather.buienradar"},
 "return_response":true}
```

Response includes an array of hourly entries with `datetime`, `condition`, `temperature`. Request after `auth_ok` and re-request periodically (e.g. every 30 min).

### ACControlScreen

Displays live AC state and allows editing target temp and mode.

**Layout** (240×240 circle):
```
    ╭────────────╮    ← lv_arc, top 180°, orange=heat / blue=cool / grey=off
         name         ← small grey label
        23.0°         ← current temp, medium grey
          21°         ← target temp, large white, center
       ⚡  HEAT       ← mode icon + label
           ←          ← go back
```

Arc fill = target temp mapped to range 16–30°C. Color driven by mode.

**Interaction states:**

| State | Dial | Button |
|---|---|---|
| `HERO` (idle) | cycles selected property: Target → Mode → Go Back | confirms selection, enters edit |
| `EDIT_TEMP` | adjusts target temp (live preview, no HA send) | sends to HA → back to HERO |
| `EDIT_MODE` | cycles modes with icons (live preview) | sends to HA → back to HERO |
| `GO_BACK` selected | — | `screenManager.pop()` |

**Dual arc:**
- Outer arc (thicker, full opacity) = target temp — color follows mode
- Inner arc (thinner, dimmed) = current temp — color follows temperature thresholds:

| Range | Color | Meaning |
|---|---|---|
| < 18°C | blue `0x0088FF` | Cold |
| 18–21°C | yellow `0xFFCC00` | Transitional |
| ≥ 21°C | orange `0xFF6600` | Warm |

Arc range: 10–30°C. Midpoint 20°C = half fill.

**Mode icons** (FontAwesome Solid via `fa_icons.h`):

| Mode | Macro | FA name |
|---|---|---|
| heat | `FA_FIRE` | fa-fire |
| cool | `FA_SNOWFLAKE` | fa-snowflake |
| auto | `FA_ARROWS_ROTATE` | fa-arrows-rotate |
| fan_only | `FA_FAN` | fa-fan |
| dry | `FA_DROPLET` | fa-droplet |
| off | `FA_POWER_OFF` | fa-power-off |

**Main menu icons:**

| Item | Macro | FA name | Rationale |
|---|---|---|---|
| Lamps | `FA_LIGHTBULB` | fa-lightbulb | — |
| Air Conditioner | `FA_WIND` | fa-wind | Unit heats in winter too — snowflake implies cooling only |
| Heater | `FA_FIRE` | fa-fire | Infrared heater, heat-only |
| Settings | `FA_GEAR` | fa-gear | — |

Icon font: FontAwesome Solid 32px, generated as `src/ui/fonts/font_awesome_solid_32.c`. See `docs/dev.md` for how to regenerate.

Selection highlight: underline bar repositioned under the active property.

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
    virtual void tick();             // called every loop; default no-op
};
```

`tick()` is forwarded by `ScreenManager` to the active screen every loop. Use it for timers or animations that need to fire independently of input or AppState changes.

### Inactivity timer

`ACControlScreen` auto-pops after 30s of no input. Implemented via `tick()`: any `onEncoder` or `onButton` call (and `show()`) resets `_lastActivityMs = millis()`; `tick()` calls `screenManager.pop()` when the delta exceeds `INACTIVITY_TIMEOUT_MS`.

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
| JSON | ArduinoJson@^7 | Standard for ESP32 JSON parsing |

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
