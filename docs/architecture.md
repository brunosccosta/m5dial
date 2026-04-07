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
struct ACState {
    const char* entity_id;
    const char* name;
    float       current_temp;
    float       target_temp;
    char        mode[12];              // "off", "heat", "cool", "heat_cool", "auto", "fan_only", "dry"
    char        availableModes[6][12]; // populated from hvac_modes on first HA snapshot
    int         modeCount;             // 0 until first HA update; use as fallback guard
    bool        valid;                 // false until first HA update
};

struct WeatherState {
    char  condition[32];         // e.g. "sunny" — weather.buienradar state
    char  detailedCondition[32]; // sensor.detailed_condition e.g. "partlycloudy-rain"
    float temperature;           // weather.buienradar a.temperature
    float feelsLike;             // weather.buienradar a.apparent_temperature
    bool  isDaytime;             // sun.sun: true = above_horizon
    bool  valid;
};

struct SensorState {
    float outdoorTemp;      // sensor.atc_3294_temperature
    float outdoorHumidity;  // sensor.atc_3294_humidity
    float bedroomTemp;      // sensor.atc_03be_temperature
    float bedroomHumidity;  // sensor.atc_03be_humidity
    float bathroomTemp;     // sensor.atc_88dc_temperature
    float bathroomHumidity; // sensor.atc_88dc_humidity
};

struct ForecastDay {
    char  detailedCondition[32]; // sensor.detailed_condition_1d / _2d
    float temperature;           // max temp — sensor.temperature_1d / _2d
    int   rainChance;            // 0–100 — sensor.rainchance_1d / _2d
    bool  valid;
};

struct SpotifyState {
    char  state[12];  // "playing" / "paused" / "idle" / "off"
    char  title[64];
    char  artist[64];
    char  source[32]; // "iPhone" / "Sala" / "Living Room"
    float volume;     // 0.0–1.0
    bool  shuffle;
    char  repeat[8];  // "off" / "one" / "all"
    bool  valid;
};

struct AppState {
    ACState      ac;               // climate.forninho_room_temperature
    ACState      heater;           // climate.forninho_portatil
    WeatherState weather;
    SensorState  sensors;
    ForecastDay  forecastToday;    // sensor.*_1d
    ForecastDay  forecastTomorrow; // sensor.*_2d
    SpotifyState  spotify;         // media_player.spotify
    MeshCoreState meshcore;        // meshcore repeater GigiTower (binary_sensor + sensors)
    bool         dirty;         // set by HA layer; cleared by UI after refresh
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

After `auth_ok`, HAClient sends `subscribe_entities` in batches of 5, one message per batch. All entity IDs come from the `SENSORS[]` registry in `HAClient.cpp`:

```json
{"id":1,"type":"subscribe_entities","entity_ids":["climate.x","weather.buienradar","sensor.a","sensor.b","sensor.c"]}
{"id":2,"type":"subscribe_entities","entity_ids":["sensor.d","sensor.e",...]}
```

Each batch gets its own message ID. `_subscribeIds[]` tracks all active IDs; the event handler accepts events from any of them.

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

`HAClient` exposes methods for service calls:

```cpp
haClient.sendACTemperature(entity_id, temp);          // climate.set_temperature
haClient.sendACMode(entity_id, mode);                 // climate.set_hvac_mode
haClient.sendFindMyIPhone(account, deviceName);       // icloud.play_sound
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

## Sensor registry

All subscribed entities and their parse handlers live in `SENSORS[]` in `src/ha/HAClient.cpp`. Adding a new sensor = one row in the table + a handler function + a field in `AppState`.

```cpp
struct SensorEntry {
    const char* entity_id;
    void (*parse)(const char* entity_id, const char* state, JsonObject attrs);
};

static const SensorEntry SENSORS[] = {
    { "climate.forninho_room_temperature", parseAC                },
    { "climate.forninho_portatil",         parseAC                },
    { "weather.buienradar",                parseWeather           },
    // ... one line per sensor
};
```

The `entity_id` is passed into every handler so shared handlers (e.g. `parseForecastCondition`) can self-route to the correct `AppState` field using the triggering entity ID.

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
- Touch (tap, no swipe) → active screen's `onTouch()` for coarse "tap anywhere" screens; LVGL `LV_EVENT_CLICKED` events for precise element targeting
- Swipe classification (main.cpp): dominant axis wins; both axes use `SWIPE_THRESHOLD = 30px`
  - Swipe down (any screen) → `QuickPanel.show()`
  - Swipe up → `QuickPanel.dismiss()` if visible, else no-op
  - Swipe left/right → `onSwipe(±1)` routed to QuickPanel or ScreenManager depending on visibility
- While `QuickPanel.isVisible()`, all input (encoder, button, touch, swipe) is routed to QuickPanel instead of ScreenManager
- RestScreen encoder → navigate cards (CCW = forward, CW = backward); resets auto-rotation timer
- RestScreen swipe left/right → navigate cards (same as encoder)
- RestScreen button or tap → push MenuScreen
- MenuScreen: encoder rotates cards, swipe left/right changes card, tap selects active card, button goes back to RestScreen
- ACControlScreen: dial adjusts target temp; tap mode pill toggles off/heat; button confirms changes (push ConfirmScreen) or pops if unchanged
- Auto-return to RestScreen after 30s inactivity on MenuScreen
- Auto-return to previous screen after 30s inactivity on ACControlScreen

**Touch architecture:**
- A `lv_indev_t` pointer input device is registered in `setup()`, reading from `M5Dial.Touch`
- LVGL uses this to dispatch `LV_EVENT_CLICKED` to objects with `LV_OBJ_FLAG_CLICKABLE` set — no manual hit testing needed
- `onTouch()` (no-op by default) is called from main.cpp on any tap; used only for coarse whole-screen tap responses (RestScreen wake, MenuScreen select active card)
- Precise touch targets (ConfirmScreen Yes/No, ACControlScreen mode pill) use `lv_obj_add_event_cb` + LVGL events

### Screen hierarchy

```
RestScreen (root)   — idle/screensaver, boots here
└── MenuScreen      — AC / Heater (card-based)
    └── ACControlScreen  — pushed by MenuCardAC / MenuCardHeater onSelect()
        └── ConfirmScreen — pushed on unsaved changes; Yes saves + pops ×2, No discards + pops ×2
```

**RestScreen** is the root of the stack — it is never popped. Encoder navigates cards; button or tap pushes MenuScreen. After 30s inactivity on MenuScreen it pops back to RestScreen.

### RestScreen

Idle/screensaver shown at boot and after 30s inactivity on the main menu. Displays at-a-glance home status. Encoder navigates between cards manually; button or tap pushes the main menu.

**Layout** (240×240 circle):

```
┌──────────────────────────────┐
│                              │
│     [ rotating card area ]   │  ← cycles every CARD_INTERVAL_MS (60s)
│                              │
│  ────────────────────────    │
│   ❄ auto 21°   🔥 heat 21°  │  ← static device strip, always visible
└──────────────────────────────┘
```

The screen is split into two zones:
- **Card area** (upper ~70%): auto-advances through a set of `RestCard` objects every 60s. Easy to add new cards.
- **Device strip** (lower ~30%): always visible — AC + heater icon, mode, target temp.

#### Card system

Each card is a `RestCard` subclass. The interface:

```cpp
class RestCard {
public:
    virtual void init(lv_obj_t* parent) = 0;  // create LVGL objects once
    virtual void update()               = 0;  // refresh labels from AppState
    virtual void show()                 = 0;  // make objects visible
    virtual void hide()                 = 0;  // hide objects
    virtual bool isVisible() const      { return true; }  // override to conditionally skip
};
```

`RestScreen` owns a `RestCard* _cards[]` array and a `_cardCount`. To add a new card: implement `RestCard`, instantiate it, add it to the array. No other changes needed.

`navigateCard(int dir)` hides the current card, walks the array by `dir` (+1 or −1) skipping cards where `isVisible()` returns false, calls `show()` + `update()` on the target card, and resets both `_lastAdvanceMs` and `_lastCardUpdateMs`. `advanceCard()` (auto-timer) calls `navigateCard(+1)`. `onEncoder(delta)` calls `navigateCard(delta > 0 ? -1 : +1)`.

`CARD_INTERVAL_MS` is a `static constexpr` in `RestScreen.h` — easy to tune.

#### Timer ring

A thin `lv_arc` sitting at the outer edge of the display, showing how much time remains until the next card. Starts full (360°) and depletes clockwise from 12 o'clock. Resets on every card advance.

```
 start / end
      │
   ───┼───          full circle at card start
  /       \
 │         │        ← arc depletes from the tail, sweeping CCW back to 12
  \       /
   ───────            empty at card end → advance fires
```

Implemented as an `lv_arc` sized 240×240, centered on the screen, on top of the card area. The knob is hidden. Both the indicator and background arcs have rounded ends.

**Configurable constants in `RestScreen.h`:**

| Constant | Default | Meaning |
|---|---|---|
| `RING_WIDTH` | `3` | Arc thickness in px |
| `RING_ACTIVE_COLOR` | `0x666666` | Depleting arc color |
| `RING_BG_COLOR` | `0x1E1E1E` | Background track color |
| `RING_START_ANGLE` | `270` | Start position in degrees (270 = 12 o'clock) |

`tick()` maps elapsed time to a 0–360 value and calls `lv_arc_set_value()` only when the integer value changes, avoiding unnecessary redraws. Paused (not updated) when `DEV_CARD_PIN` is set.

#### Cards

| # | Card | Data sources | `isVisible()` |
|---|---|---|---|
| 0 | `ClockCard` | system time via `localtime()` (seeded from RTC on boot, synced from NTP) | always |
| 1 | `WeatherNowCard` | `weather.buienradar` + `sensor.detailed_condition` + `sun.sun` | always |
| 2 | `IndoorTempsCard` | `sensor.atc_3294/03be/88dc` temp + humidity (balcony, bedroom, bathroom) | always |
| 3 | `ForecastCard` | `sensor.*_1d` / `sensor.*_2d` Buienradar forecast sensors | always |
| 4 | `SpotifyCard` | `media_player.spotify` — title, artist, source, volume, shuffle, repeat | only when state is `"playing"` or `"paused"` |
| 5 | `MeshCoreCard` | MeshCore repeater GigiTower — battery %, 24h trend, uptime, last-updated | always |

#### MeshCoreCard layout

```
┌──────────────────────────────┐
│         [tower icon]         │  ← FA_TOWER_BROADCAST (18px), y=-80, centered
│         GigiTower            │  ← montserrat_28, y=-50, centered
│                              │
│ 🔋 65% ▲ +2.1%  ⏱ 21d 10h  │  ← stats row, y=-16
│  ● Last updated 3h ago       │  ← dot + montserrat_14 (TEXT_DIM), y=+12
└──────────────────────────────┘
```

**Y offsets (from screen center):** icon −80, name −50, stats row −16, status +12.

**Stats columns:** bat icon `x=−85`, bat% `x=−57`, trend caret `x=−33`, diff `x=−7`, uptime icon `x=+30`, uptime value `x=+70`.

**Battery trend:** queried once on connect via `history/history_during_period` (last 24h). Caret + diff colored green (`0x00CC44`) for positive, red (`0xFF3333`) for negative. Hidden until history result arrives.

**Status dot:** 8px `lv_obj` circle positioned dynamically to the left of the "Last updated" label (`lv_obj_update_layout` + `lv_obj_get_width`). Green ≤6h, yellow 6–12h, red >12h, grey when no data.

**Status row:** "Last updated Xh ago" from battery sensor's `last_updated` attribute (local time, no Z suffix — parsed with `mktime`, same TZ as device clock). Falls back to `"Last updated --"` when clock not synced.

**Uptime formatting:** HA state is fractional days. Parser converts to `uptimeSeconds`. Card formats as `"Xd Yh"` / `"Xh Ym"` / `"Xm"`.

**Fonts:** `font_awesome_solid_18` for all icons; `lv_font_montserrat_28` for name; `lv_font_montserrat_14` for stat values and status.

#### ClockCard layout

```
┌──────────────────────────────┐
│                              │
│          14:32               │  ← montserrat_48, y=−45, centered
│        Wed 5 Mar             │  ← montserrat_24 (TEXT_DIM), y=−3, centered
│                              │
└──────────────────────────────┘
```

#### WeatherNowCard layout

```
┌──────────────────────────────┐
│                              │
│    8°  / 5°                  │  ← temp montserrat_48 at x=−9, y=−45;
│                              │    feels-like montserrat_14 anchored to temp right-bottom
│    ☁  Partly Cloudy          │  ← icon (FA 18px) + label (montserrat_24, TEXT_FAINT), y=−8
│                              │    group dynamically centered; scrolls circular if >200px
└──────────────────────────────┘
```

Icon + condition label are measured on every `update()` via `lv_obj_update_layout()` and repositioned so the combined group center aligns with screen center. If total width > 200px, label is capped and switches to `LV_LABEL_LONG_SCROLL_CIRCULAR` (14s).

#### IndoorTempsCard layout

```
┌──────────────────────────────┐
│  ☀  21.3°   💧  55%          │  ← balcony row, y=−61
│  🛏  19.1°   💧  48%         │  ← bedroom row, y=−28
│  🚿  22.0°   💧  62%         │  ← bathroom row, y=+5
└──────────────────────────────┘
```

Columns: icon `x=−61`, temp `x=−15` (montserrat_24), droplet `x=+30` (FA 18px, ACCENT_RAIN), humidity `x=+56` (montserrat_14, TEXT_DIM).

#### ForecastCard layout

```
┌──────────────────────────────┐
│   TODAY        TMR           │  ← montserrat_14 (TEXT_MUTED), y=−73
│     ☁            ☀           │  ← FA icon 24px, y=−45
│    12°          15°          │  ← montserrat_28, y=−16
│  🌧  40%      🌧  10%        │  ← rain icon (FA 18px) + montserrat_14, y=+13
└──────────────────────────────┘
```

Two columns at `x=−37` (today) and `x=+41` (tomorrow). All positions static.

#### SpotifyCard layout

```
┌──────────────────────────────┐
│           playing            │  ← state, montserrat_14 (TEXT_MUTED), y=−77, centered
│        Song Title            │  ← montserrat_24, y=−40, w=180, scroll circular (19s)
│         Artist               │  ← montserrat_14 (TEXT_DIM), y=−12, w=180, scroll (14s)
│  📱 iPhone   🔊 75%          │  ← source icon+label left (x=−82/−29), vol icon+label right (x=+40/+76), y=+14
│       🔀        🔁           │  ← shuffle (x=−12) + repeat (x=+12), y=+36; hidden when inactive
└──────────────────────────────┘
```

`isVisible()` returns true only when state is `"playing"` or `"paused"`. Source icon maps: iPhone→`FA_MOBILE`, Sala→`FA_TOWER_BROADCAST`, Living Room→`FA_TV`.

Adding a card = new `.h`/`.cpp` file + one line in `RestScreen`'s card array. Card content and layout are fully self-contained.

#### NTP and RTC time sync

`configTime()` + `setenv("TZ", ...)` are called in `setup()`. SNTP syncs in the background once WiFi connects.

On boot, if the BM8563 RTC is enabled and has no power-failure flag (`getVoltLow() == true`) and year ≥ 2020, `setSystemTimeFromRtc()` seeds system time immediately — so the clock shows without waiting for WiFi.

Once NTP syncs (`time() > Jan 2020`), the loop writes UTC back to the RTC via `setDateTime(gmtime(&t))` once. RTC always stores UTC; `localtime()` applies the TZ rule for display.

#### Data sources

| Field | Source |
|---|---|
| Current condition | `appState.weather.condition` |
| Detailed condition | `appState.weather.detailedCondition` |
| Current temp | `appState.weather.temperature` |
| Feels like | `appState.weather.feelsLike` |
| Outdoor temp | `appState.weather.outdoorTemp` |
| Outdoor humidity | `appState.weather.outdoorHumidity` |
| Day/night | `appState.weather.isDaytime` |
| Forecast today | `appState.forecastToday` |
| Forecast tomorrow | `appState.forecastTomorrow` |
| AC status | `appState.ac` |
| Heater status | `appState.heater` |

**Forecast data** comes from Buienradar's per-day sensor entities (`sensor.detailed_condition_1d`, `sensor.temperature_1d`, `sensor.rainchance_1d`, etc.) subscribed via the same `subscribe_entities` call. These must be enabled in HA (disabled by default).

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

### ACControlScreen

Displays live AC state and allows editing target temp and mode.

**Layout** (240×240 circle):
```
    ╭────────────╮    ← dual lv_arc (outer = target temp, inner = current temp)
        23.0°         ← current temp, small grey
          21°         ← target temp, large white, center
       🔥  HEAT       ← mode pill (clickable container); tap to toggle off/heat
```

**Interaction:**
- Dial → adjusts target temp live (±0.5°C per step, clamped 10–30°C)
- Tap mode pill → toggles off ↔ heat (LVGL click event on `_modeContainer`)
- Button → if changed: push `ConfirmScreen`; if unchanged: pop to MenuScreen
- 30s inactivity → pop to MenuScreen

**Change detection:** `_originalTemp` and `_originalMode` captured in `show()`. Compared on button press.

**Dual arc:**
- Outer arc (10px) = target temp — `Theme::AC_MODE_HEAT` when heating, `Theme::RING_BG` when off
- Inner arc (5px, dimmed) = current temp — color follows temperature thresholds:

| Range | Color |
|---|---|
| < 18°C | `Theme::TEMP_COLD` |
| 18–21°C | `Theme::TEMP_MID` |
| ≥ 21°C | `Theme::TEMP_WARM` |

Arc range: 10–30°C.

**Mode icons** (FontAwesome Solid via `fa_icons.h`):

| Mode | Macro | FA name |
|---|---|---|
| heat | `FA_FIRE` | fa-fire |
| cool | `FA_SNOWFLAKE` | fa-snowflake |
| heat_cool / auto | `FA_ARROWS_ROTATE` | fa-arrows-rotate |
| fan_only | `FA_FAN` | fa-fan |
| dry | `FA_DROPLET` | fa-droplet |
| off | `FA_POWER_OFF` | fa-power-off |

Mode is cycled by tapping the mode pill. Uses `_ac->availableModes[]` from HA when populated; falls back to a static 6-mode list.

**Menu card icons:**

| Item | Macro | FA name | Rationale |
|---|---|---|---|
| AC | `FA_WIND` | fa-wind | Unit heats in winter too — snowflake implies cooling only |
| Heater | `FA_FIRE` | fa-fire | Infrared heater, heat-only |

Icon font: FontAwesome Solid, generated as `font_awesome_solid_32.c` (32px), `font_awesome_solid_24.c` (24px), and `font_awesome_solid_18.c` (18px). See [`src/ui/fonts/README.md`](../src/ui/fonts/README.md) for how to add glyphs and regenerate.

### ConfirmScreen

Generic confirmation screen. Push after calling `setup()` with question text and yes/no callbacks.

```cpp
confirmScreen.setup("Save changes?",
    []() { /* yes: apply + pop ×2 */ },
    []() { /* no:  discard + pop ×2 */ }
);
screenManager.push(&confirmScreen);
```

**Layout:** Question centered, Yes (left) and No (right) below with icons (`LV_SYMBOL_OK` / `LV_SYMBOL_CLOSE`).

**Interaction:**
- Tap Yes or No → confirms immediately (LVGL click events on each container)
- Dial → highlights an option (underline); no highlight by default — encourages touch
- Button → confirms highlighted option; defaults to No if dial was never touched
- No inactivity timer

### MenuScreen

Card-based main menu. Implements `Screen`. Each `MenuCard` is a preview panel that can push a control screen.

**Card interface:**
```cpp
class MenuCard {
    virtual void        init(lv_obj_t* container) = 0; // create LVGL objects in 240x240 container
    virtual void        update()                  = 0; // refresh from AppState
    virtual const char* label()                   = 0; // display name
    virtual const char* icon()                    = 0; // FA glyph
    virtual void        onSelect()                = 0; // called on touch/select — push control screen
};
```

**Arc position indicator:** The ring arc (same width/style as the RestScreen timer ring) is split into N equal segments with 5° gaps. Active segment = `Theme::RING_ACTIVE`; others = `Theme::RING_BG`. Implemented as N separate `lv_arc` objects, each spanning its segment, with `RING_ACTIVE` as indicator (value=1 on active, 0 on others) and `RING_BG` as track (always visible).

**Card transitions:** Horizontal slide via `lv_anim` on container x position (200ms). Outgoing card slides off, incoming slides in. Input is blocked during animation.

**Input:**
- Encoder → rotate between cards (wraps around)
- Touch → `onSelect()` on active card → pushes control screen
- Button → `screenManager.pop()` (back to rest)
- 30s inactivity → `screenManager.pop()`

Concrete cards: `MenuCardAC` (used for both AC and Heater, with constructor args), `MenuCardFindMy` (fires `icloud.play_sound` then pops).

### ToastOverlay

Transient feedback pill shown on `lv_layer_top()` — always above all screens. Auto-dismisses after a configurable duration.

```cpp
toast.show(FA_MOBILE, "Ringing...", 2000); // icon, message, duration ms
```

**Layout:** 200×64 rounded pill (`Theme::SURFACE`), flex row centered — FA icon (24px) + message text (montserrat_24). Fades in over 200ms, fades out over 300ms when duration expires.

`init()` is called once in `setup()`. `update()` is called every loop to trigger the fade-out when the timer expires.

---

### QuickPanel

System-level overlay — iPhone Control Center style. Lives on `lv_layer_top()` above all screens. Triggered by swipe-down from any screen; dismissed by swipe-up or button press.

**Animation:** slides y from −240→0 on show (200ms ease-out), 0→−240 on dismiss (150ms ease-in), then hidden.

**Input gating:** while `isVisible()`, main.cpp routes all input (encoder, button, touch, swipe) to `QuickPanel` instead of `ScreenManager`.

**MVP layout:**
```
┌──────────────────────────────┐
│   ● WiFi  ● HA    HA Ready   │  connection row (y≈55)
│  ☀ 21°   🛏 19°   🚿 22°     │  indoor temps  (y≈110)
│      [ 📱  Find iPhone ]     │  action pill   (y≈160)
└──────────────────────────────┘
```

Connection dots: green = connected, yellow = partial, red = disconnected.

**`FindMyAction`** — shared coordinator (`src/ha/FindMyAction.h`) used by both `QuickPanel` and `MenuCardFindMy`. Holds account/deviceName; `trigger()` calls `haClient.sendFindMyIPhone()` + `toast.show()`. Keeps the two callers DRY.

---

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
    virtual void onTouch();          // called on any touch; default no-op
    virtual void refresh();          // called when AppState dirty
    virtual void tick();             // called every loop; default no-op
};
```

`tick()` is forwarded by `ScreenManager` to the active screen every loop. Use it for timers or animations that need to fire independently of input or AppState changes.

### Inactivity timer

`ACControlScreen` auto-pops after 30s of no input. Implemented via `tick()`: any `onEncoder` or `onButton` call (and `show()`) resets `_lastActivityMs = millis()`; `tick()` calls `screenManager.pop()` when the delta exceeds `INACTIVITY_TIMEOUT_MS`.

Each screen owns its own `lv_obj_t* _lvScreen` (LVGL screen object). `show()` calls `lv_scr_load`.

---

### Theming

All UI colors are defined as named constants in `src/ui/themes/ThemeDefault.h` and accessed via `src/ui/Theme.h`. UI code never contains raw hex values.

Active theme is selected at compile time via `platformio.ini`:
```ini
-DACTIVE_THEME=1   ; 1 = ThemeDefault
```

See [`src/ui/themes/README.md`](../src/ui/themes/README.md) for the full token reference and instructions for adding a new theme.

---

## Loop structure

```cpp
void loop() {
    M5Dial.update();
    haClient.update();   // non-blocking HA/WiFi processing

    // Input handled BEFORE lv_timer_handler so swipe cancellation
    // (suppressNextRelease + lv_indev_reset) takes effect before LVGL
    // processes touch events and fires LV_EVENT_CLICKED.
    input.update();
    // ... encoder / button / touch / swipe classification

    lv_timer_handler();  // LVGL rendering + event dispatch
    delay(5);

    // overlays, tick, dirty check
}
```

### Swipe cancellation

When a gesture is classified as a swipe, `cancelTouch()` is called before `lv_timer_handler()`:

```cpp
static void cancelTouch() {
    suppressNextRelease = true;          // belt-and-suspenders: report release at (0,0)
    lv_indev_reset(touchIndev, nullptr); // clears last_pressed so LV_EVENT_CLICKED never fires
}
```

`lv_indev_reset` is the real fix — LVGL dispatches `LV_EVENT_CLICKED` to `last_pressed` regardless of where the finger lifts, so suppressing coordinates alone is not sufficient.

For QuickPanel dismiss specifically, an early-dismiss path fires on `isPressed()` (mid-gesture) rather than `wasReleased()`, so the panel slides away before the finger lifts. `gestureHandled` prevents double-classification of the same touch.

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
