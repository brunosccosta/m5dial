# Learnings

Project-specific findings, gotchas, and non-obvious fixes. Saves time when revisiting or debugging regressions.

---

## M5Dial hardware init

`M5Dial.begin()` with no arguments does **not** initialize the encoder — its internal state pointer stays null and crashes on first `Encoder.read()`.

**Fix**: always pass explicit args:
```cpp
auto cfg = M5.config();
M5Dial.begin(cfg, true, false); // (config, enable_encoder, enable_touch)
```

---

## LVGL v9 — display setup API changed from v8

The entire display driver registration API was removed in v9. Old v8 code (`lv_disp_drv_t`, `lv_disp_draw_buf_t`, etc.) will compile but crash at runtime.

**v9 equivalent**:
```cpp
lv_display_t *disp = lv_display_create(240, 240);
lv_display_set_flush_cb(disp, my_flush_cb);

static lv_color_t buf[240 * 24];
lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
```

Flush callback signature also changed:
```cpp
// v8: void flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
// v9:
void flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
```

And `lv_disp_flush_ready(drv)` → `lv_display_flush_ready(disp)`.

---

## `lv_label_set_text_fmt` does not support `%f`

LVGL's internal formatter does not handle float format specifiers. Using `%.0f` or `%f` in `lv_label_set_text_fmt` outputs the literal `f` character instead of a number.

**Fix**: use `snprintf` into a local buffer, then `lv_label_set_text`:
```cpp
char buf[32];
snprintf(buf, sizeof(buf), "%.1f°", value);
lv_label_set_text(label, buf);
```

---

## LVGL Montserrat font sizes — must be explicitly enabled in platformio.ini

LVGL ships Montserrat in many sizes but only compiles those explicitly enabled via build flags. Using an unenabled size compiles but renders garbage — it reads uninitialised memory.

Enable in `platformio.ini`:
```ini
-DLV_FONT_MONTSERRAT_14=1
-DLV_FONT_MONTSERRAT_24=1
-DLV_FONT_MONTSERRAT_28=1
-DLV_FONT_MONTSERRAT_32=1
-DLV_FONT_MONTSERRAT_48=1
```

---

## LVGL partial refresh doesn't trigger automatically

`lv_label_set_text()` marks the object dirty, but `lv_timer_handler()` alone may not flush it to the display in all cases.

**Fix**: call `lv_refr_now(NULL)` after updating labels to force an immediate render+flush.

---

## ESP32-S3 Serial goes to UART0, not USB CDC by default

On ESP32-S3, `Serial.printf()` goes to UART0 (physical pins). The USB-C port (what PlatformIO monitor connects to) is USB CDC — a different interface.

**Fix**: add to `platformio.ini` build_flags:
```
-DARDUINO_USB_CDC_ON_BOOT=1
```
This remaps `Serial` to USB CDC.

---

## Logging setup

Using ESP-IDF's `esp_log` system (already available, no extra library):
- Include: `#include <esp_log.h>`
- Control level via `platformio.ini`: `-DCORE_DEBUG_LEVEL=N` (0=off, 4=debug, 5=verbose)
- Default: `CORE_DEBUG_LEVEL=1` (errors only)

See [docs/architecture.md logging section](architecture.md) and `agents.md` for the full convention.

---

## GC9A01 color channels swapped (RGB vs BGR)

LVGL v9 outputs RGB565 by default; the GC9A01 display expects bytes in the opposite order. Red appears as blue and vice versa.

**Fix**: declare the swapped format right after `lv_display_create`:
```cpp
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
```
Note: the constant is `_SWAPPED` (not `_SWAP`) in this version of LVGL.

---

## Circular display: positions outside r=120 are silently clipped

The GC9A01 is a 240×240 circular display (center 120,120, radius 120). Any pixel outside the circle is simply not rendered — no error, no wrapping. `lv_obj_align(LV_ALIGN_TOP_RIGHT)` places objects in the square bounding box corner, which is outside the circle.

**Fix**: use `lv_obj_set_pos()` with explicit coordinates and verify: `sqrt((x+w/2 - 120)² + (y+h/2 - 120)²) < 120`.

For a "1 o'clock" dot: `lv_obj_set_pos(_dot, 169, 19)` → center at (175, 25), distance ≈ 106px.

---

## `lv_obj_align` positions are not committed until layout is resolved

`lv_obj_get_x/y/width/height` return stale values (often 0) immediately after `lv_obj_align` — LVGL resolves layout lazily. Reading positions before a redraw cycle gives wrong results.

**Fix**: call `lv_obj_update_layout(screen)` before reading any positions:
```cpp
lv_scr_load(_lvScreen);
lv_obj_update_layout(_lvScreen);
// now lv_obj_get_x/y are correct
```

This also applies to `lv_obj_align_to` — the base object's size must be resolved first or the relative position will be wrong:
```cpp
lv_obj_align(_icon, LV_ALIGN_CENTER, -20, 53);
lv_obj_update_layout(_lvScreen);
lv_obj_align_to(_text, _icon, LV_ALIGN_OUT_RIGHT_MID, 6, 0); // now icon width is known
```

---

## Mixing fonts requires separate label objects

LVGL labels use a single font — there's no inline font switching within one label. To render an icon glyph (FA font) next to text (Montserrat) in the same row, create two labels and position the second relative to the first with `lv_obj_align_to`:

```cpp
lv_obj_set_style_text_font(_iconLabel, &font_awesome_solid_18, LV_PART_MAIN);
lv_obj_set_style_text_font(_textLabel, &lv_font_montserrat_14, LV_PART_MAIN);

lv_obj_align(_iconLabel, LV_ALIGN_CENTER, -20, y);
lv_obj_update_layout(screen);
lv_obj_align_to(_textLabel, _iconLabel, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
```

---

## Grouping LVGL objects for show/hide — use a transparent container

To show or hide a set of labels together (e.g. a card that replaces another), parent them all to a single transparent `lv_obj_t` container. Toggling `LV_OBJ_FLAG_HIDDEN` on the container propagates to all children automatically.

```cpp
_container = lv_obj_create(parent);
lv_obj_set_size(_container, 240, 240);   // match screen size so child coords stay in screen space
lv_obj_set_pos(_container, 0, 0);
lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);        // avoid child offset
lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

// then show/hide the whole group:
lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); // show
lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); // hide
```

`pad_all(0)` is important — default LVGL padding shifts children away from the container origin, making `LV_ALIGN_CENTER` misalign relative to the screen center.

---

## `lv_obj_align` on `lv_layer_top()` children — use `lv_obj_set_pos` instead

`lv_obj_align` with parent-relative alignments (e.g. `LV_ALIGN_TOP_RIGHT`) may not resolve correctly for children of `lv_layer_top()` if the layer's size isn't explicitly set. The object ends up at position (0,0) or wrong coordinates.

**Fix**: use `lv_obj_set_pos()` with absolute pixel coordinates for anything parented to a system layer.

---

## WebSockets library (Links2004)

- Non-blocking: call `_ws.loop()` every loop tick (inside `HAClient::update()`)
- `setReconnectInterval(ms)` — library retries automatically; no manual reconnect logic needed
- `enableHeartbeat(pingMs, pongTimeoutMs, retries)` — built-in WS ping/pong keepalive
- HA sends `auth_required` on every new connection, so auth flow repeats automatically after reconnect

## `subscribe_entities` batch ID tracking — don't cap at a fixed MAX_BATCHES

The original implementation stored each subscription batch's message ID in a fixed array (`_subscribeIds[MAX_BATCHES]`) and rejected events whose ID wasn't in the array. When the number of sensors grew past `MAX_BATCHES * BATCH_SIZE`, the extra batches' IDs were silently not stored — their events were dropped with no log. The sensor appeared to receive data (the HA event was logged) but `AppState` was never updated.

**Fix**: subscription IDs are always sequential starting from 1 (since `_msgId` resets to 0 on each `subscribeEntities()` call). Replace the array with a count and a range check:

```cpp
// In subscribeEntities():
_subscribeCount++; // was: if (_subscribeCount < MAX_BATCHES) _subscribeIds[_subscribeCount++] = id;

// In handleMessage() event handler:
if (id < 1 || id > _subscribeCount) return; // was: loop over _subscribeIds[]
```

No array, no cap — works for any number of sensors.

---

## `get_states` disconnects the WS — use `subscribe_entities` instead

`get_states` returns all HA entities in a single WS frame. For a typical HA install this is 50–200KB. The Links2004 WebSocketsClient library drops the connection when the incoming frame exceeds its buffer — no error, just `WStype_DISCONNECTED` immediately after the frame arrives.

**Fix**: use `subscribe_entities` with explicit `entity_ids`. HA sends only the requested entities (initial snapshot + incremental diffs), keeping frames small.

```json
{"id":1,"type":"subscribe_entities","entity_ids":["climate.your_entity"]}
```

---

## `subscribe_entities` event format is compressed

Unlike `state_changed` events, `subscribe_entities` uses a compact diff format:

- `event.a` (added) — initial snapshot, full state + attributes
- `event.c` (changed) — only changed fields, nested under `"+"`

Fields absent in a diff mean unchanged — don't overwrite them with defaults.

---

## weather.buienradar — no forecast in subscribe_entities

`subscribe_entities` for `weather.buienradar` returns current conditions only (`supported_features: 1`). The forecast array is **not** included. Hourly forecast requires a separate `weather.get_forecasts` call with `return_response: true`.

Fields available via subscribe: `s` (condition), `apparent_temperature` (feels like), `temperature`, `humidity`, `wind_speed`, `wind_bearing`, `pressure`.

---

## Uninitialized LVGL labels render default text through the active font

`lv_label_create()` sets a default text string. If the label uses a symbol font (e.g. FontAwesome) and its text is never explicitly set, LVGL renders the default text through that font — producing colored squares for every character that has no glyph.

**Fix**: always call `lv_label_set_text()` before the label is shown, even for static content that never changes. For icons that are constant, set the text in `init()`.

```cpp
// Wrong — text never set; renders as colored boxes
_rainIconLeft = lv_label_create(_container);
lv_obj_set_style_text_font(_rainIconLeft, &font_awesome_solid_18, LV_PART_MAIN);

// Correct
lv_label_set_text(_rainIconLeft, FA_DROPLET);
```

---

## ACState::entity_id must be set by the parse handler

`ACState` holds a `const char* entity_id` pointer used by `sendACMode` / `sendACTemperature`. As a global struct it zero-inits to `nullptr`. The parse handler receives `entity_id` as a parameter but the old code never wrote it back to the struct — so the first send after boot crashes with a `LoadProhibited` null-deref.

**Fix**: set `ac.entity_id` to the known string literal inside `parseAC()`:
```cpp
bool isMain = strcmp(entity_id, "climate.forninho_room_temperature") == 0;
ACState& ac  = isMain ? appState.ac : appState.heater;
ac.entity_id = isMain ? "climate.forninho_room_temperature" : "climate.forninho_portatil";
```
Storing a string literal is safe — it has static storage duration.

---

## M5Dial has no vibration motor

The M5Dial spec lists: encoder, buzzer, RFID, touch, display. No haptic/vibration motor. An I2C scan of the internal bus (GPIO 11/12) found `0x38` (FT3267 touch), `0x51` (BM8563 RTC), `0x28` (unknown — likely AW8624 buzzer amp) — no DRV2605L at `0x5A`. The DRV2605L is not present.

---

## Compiled LVGL fonts are expensive flash

Each enabled Montserrat size adds ~45KB of flash. Only enable sizes you actually use.

Dropping `montserrat_32` (one use, migrated to `montserrat_28`) saved **45,924 bytes** of flash with no RAM impact — fonts live in flash only.

Audit with:
```bash
grep -rn "lv_font_montserrat_" src/ | sed 's/.*montserrat_\([0-9]*\).*/\1/' | sort | uniq -c
```

Cross-check against the sizes enabled in `platformio.ini` build flags.

---

## LVGL click events require a registered indev

`lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, data)` silently does nothing unless an `lv_indev_t` pointer device is registered. LVGL only dispatches click/press/release events to objects when it knows about touch input via an indev.

**Fix**: register a pointer indev in `setup()` before any screens are pushed:
```cpp
lv_indev_t* indev = lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(indev, my_touch_read_cb);
```

The read callback reports `isPressed()` + `x`/`y` from `M5Dial.Touch.getDetail()`.

---

## media_player.spotify — available fields

Fields confirmed via `subscribe_entities` snapshot (state + attrs):

| Field | Type | Example | Notes |
|---|---|---|---|
| state | string | `"playing"` | `"playing"` / `"paused"` / `"idle"` / `"off"` |
| `media_title` | string | `"Too Sweet"` | — |
| `media_artist` | string | `"Hozier"` | — |
| `media_album_name` | string | `"Unreal Unearth: Unaired"` | — |
| `media_duration` | int | `251` | seconds |
| `media_position` | int | `149` | seconds; stale — interpolate using `media_position_updated_at` |
| `media_position_updated_at` | string | `"2026-04-02T19:26:17.821560+00:00"` | UTC ISO8601 |
| `source` | string | `"iPhone"` | playback device |
| `shuffle` | bool | `false` | — |
| `repeat` | string | `"off"` | `"off"` / `"one"` / `"all"` |
| `volume_level` | float | `1.0` | 0.0–1.0 |
| `entity_picture` | string | `/api/media_player_proxy/...` | HA-proxied album art URL — rendering requires HTTP fetch + JPEG decode |

`entity_picture` is technically available but non-trivial: requires an HTTP GET to HA + JPEG decoding into an LVGL image buffer.

---

## Debugging new HA entities — subscribe + raw log

To inspect what a new HA entity sends before writing a real parser:

1. Add a temporary `parseXxxDebug` function that logs state + serialises attrs:
```cpp
static void parseXxxDebug(const char* entity_id, const char* state, JsonObject attrs) {
    ESP_LOGI(TAG, "XXX state=%s", state ? state : "(null)");
    if (!attrs.isNull()) {
        char buf[512];
        serializeJson(attrs, buf, sizeof(buf));
        ESP_LOGI(TAG, "XXX attrs=%s", buf);
    }
}
```
2. Add a row to `SENSORS[]` pointing at it.
3. Flash and read serial. Look for the log lines to see exactly what fields HA sends.
4. Remove the debug parser once the real one is in place.

If nothing appears, the entity ID probably doesn't match — check the exact ID in HA developer tools and update the SENSORS entry.

---

## BM8563 RTC — getVoltLow() polarity and UTC storage

`getVoltLow()` returns `true` = **no** power failure (RTC data is valid). Returns `false` = power failure occurred (data unreliable). The name is misleading — treat `true` as the "safe to use" case.

Always store UTC in the RTC (`setDateTime(gmtime(&t))`), not local time. `setSystemTimeFromRtc()` sets system UTC, and `localtime()` applies the TZ rule. Storing local time breaks across DST transitions.

```cpp
// Write NTP time to RTC (UTC):
M5Dial.Rtc.setDateTime(gmtime(&t));

// On boot, seed system time from RTC:
if (M5Dial.Rtc.isEnabled() && M5Dial.Rtc.getVoltLow()) {
    auto dt = M5Dial.Rtc.getDateTime();
    if (dt.date.year >= 2020)
        M5Dial.Rtc.setSystemTimeFromRtc();
}
```

---

## LV_LABEL_LONG_SCROLL_CIRCULAR drops text alignment

Switching a label from `LV_LABEL_LONG_DOT` to `LV_LABEL_LONG_SCROLL_CIRCULAR` does not carry over `lv_obj_set_style_text_align`. The style must be re-applied after setting the new long mode, otherwise short text (that doesn't scroll) defaults to left-aligned within the label's bounding box.

```cpp
lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN); // must re-set
```

Scroll speed is controlled by `lv_obj_set_style_anim_duration(label, ms, LV_PART_MAIN)`. The value is the full cycle duration — the text scrolls from start back to start in that time. ~15–19 seconds feels natural for a rest card.

---

## LVGL click fires on last-pressed object regardless of release coordinates

`LV_EVENT_CLICKED` is dispatched to `last_pressed` — the object that received the initial `LV_EVENT_PRESSED` — not to whatever object is under the finger at release. Reporting the release at a neutral point (e.g. 0,0) via the indev read callback does **not** suppress the click; LVGL still sends `LV_EVENT_RELEASED` / `LV_EVENT_CLICKED` to the previously-pressed object.

**Fix**: call `lv_indev_reset(indev, nullptr)` when a gesture is classified as a swipe. This clears `last_pressed` and `act_obj` so LVGL has no object to deliver events to.

```cpp
// Store the indev pointer globally at setup time:
touchIndev = lv_indev_create();
lv_indev_set_type(touchIndev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(touchIndev, my_touch_read);

// On swipe detection (before lv_timer_handler runs):
lv_indev_reset(touchIndev, nullptr);
```

Suppressing the release coordinates is still useful as a belt-and-suspenders fallback but is not sufficient on its own.

---

## LVGL v9 image API — different from v8

In v9, the image widget and descriptor types were renamed:

| v8 | v9 |
|---|---|
| `lv_img_dsc_t` | `lv_image_dsc_t` |
| `lv_img_create(parent)` | `lv_image_create(parent)` |
| `lv_img_set_src(obj, &dsc)` | `lv_image_set_src(obj, &dsc)` |

The generated C files from `LVGLImage.py --ofmt C` use `lv_image_dsc_t` — this is the correct v9 type. Using the old `lv_img_*` names compiles but may link to deprecated stubs or fail entirely.

**`--premultiply` is required for correct emoji rendering.** Emoji PNGs have semi-transparent edges. Without premultiplied alpha, LVGL blends the raw alpha incorrectly — edges appear frayed or with a dark halo. Always pass `--premultiply` when converting RGBA images.

---

## Bottom-aligning two labels of different font sizes

LVGL's `LV_ALIGN_CENTER` aligns the center of an object. When placing a small label beside a large one (e.g. "22nd May" in montserrat_14 next to "45 days" in montserrat_24), centering both at the same y produces top-alignment visually — the small label hangs above the baseline of the large one.

To bottom-align: measure both heights after `lv_obj_update_layout`, then offset the smaller label's y so its bottom matches the larger one's bottom:

```cpp
lv_obj_update_layout(_container);
int bigH   = lv_obj_get_height(_bigLabel);    // e.g. 28px
int smallH = lv_obj_get_height(_smallLabel);  // e.g. 18px
int bigY   = -5;                              // center of big label
int smallY = bigY + bigH / 2 - smallH / 2;   // bottom-align small to big
lv_obj_align(_bigLabel,   LV_ALIGN_CENTER, bigX,   bigY);
lv_obj_align(_smallLabel, LV_ALIGN_CENTER, smallX, smallY);
```

`lv_obj_update_layout` must be called **after** setting label text — heights are only valid once layout is resolved.

---

## ESP32 `Preferences::begin()` with `readOnly=true` fails if namespace doesn't exist

On first boot (or after NVS erase), opening a `Preferences` namespace with `readOnly = true` returns `NOT_FOUND` — the namespace hasn't been created yet. `getString` is never reached, and the fallback never fires.

**Fix**: always open with `readOnly = false`. If the key is absent, `getString(key, default)` safely returns the default. No data is written unless `putString` is explicitly called.

---

## Typographic apostrophes from Spotify/device metadata break LVGL rendering

macOS device names and Spotify track metadata use Unicode typographic apostrophes (U+2018 `'`, U+2019 `'`) instead of ASCII `'`. These are 3-byte UTF-8 sequences (`0xE2 0x80 0x98/99`). LVGL's compiled Montserrat font doesn't include these code points — they render as garbage or boxes.

**Fix**: sanitize strings on ingestion with a simple in-place replacer before storing to AppState. Applied to `media_title`, `media_artist`, and `source` in `parseSpotify()` via `sanitizeForDisplay()` in `HAClient.cpp`.

---

## Spotify API — `context_uri` vs `uris` for playback

`PUT /v1/me/player/play` uses different body fields depending on what you're playing:

- **Albums, playlists, artists, shows**: `{"context_uri":"spotify:album:..."}`
- **Tracks, episodes**: `{"uris":["spotify:track:..."]}` — must be an array, `context_uri` returns 400

Using `context_uri` with a track URI returns HTTP 400 with no useful error message.

---

## NTAG213 NDEF read buffer must cover the full URI length

`MIFARE_Read` reads 4 pages (16 bytes) per call. NDEF text payload starts at page 4, offset 9 (after TLV + record header + lang). A `spotify:device:<40-char-id>` URI is 55 bytes — plus 9 bytes overhead = 64 bytes total from page 4, which spans pages 4–19.

Reading only pages 4–15 (48 bytes) truncates and corrupts any URI longer than ~39 bytes. The tag appears to read successfully (no error) but the payload buffer contains garbage past the read boundary.

**Fix**: read pages 4–23 (80 bytes, 5 MIFARE_Read calls). Covers any Spotify URI with room to spare. NTAG213 user memory goes to page 39 so this is well within bounds.

---

## `media_player.spotify` does not support `play_media`

The HA Spotify integration entity (`media_player.spotify`) is a read/tracking entity — it reports state, title, artist, source, volume, etc. It does **not** accept `media_player.play_media` service calls; HA returns:

```
service_validation_error: Entity media_player.spotify does not support action media_player.play_media
```

**Fix**: use the Spotify Web API directly (`PUT /me/player/play` with `context_uri`). This targets the currently active Spotify device — works regardless of whether it's a phone, BT speaker, or Spotify Connect device. Requires OAuth Authorization Code flow once to get a refresh token.

---

## NTAG213 NDEF TLV layout (confirmed 2026-04-12)

NTAG213 user memory starts at page 4. A Text record written by NFC Tools has this exact layout:

```
page 04: 03 <len> D1 01    — TLV type=NDEF (0x03), 1-byte length, record header (MB|ME|SR), type_len=1
page 05: <payload_len> 54 02 <lang[0]>  — payload length (SR=1→1 byte), type='T', status byte (lang_len=2, UTF-8), lang start
page 06: <lang[1]> <text...>            — lang byte 2, then text payload
```

TLV parse order: type (0x03) → length → NDEF header → type_len → payload_len (1 byte if SR flag set) → skip ID if IL flag → type byte ('T') → status byte → skip lang → read text.

No external library needed. MIFARE Classic tags (`PICC_TYPE_MIFARE_1K` etc.) require `PCD_Authenticate` before any read and are irrelevant to this project — ignore them.

---

## HA WebSocket auth flow

Fixed protocol on connect — no variation:
1. Server → `{"type":"auth_required","ha_version":"..."}`
2. Client → `{"type":"auth","access_token":"TOKEN"}`
3. Server → `{"type":"auth_ok"}` or `{"type":"auth_invalid"}`

`auth_invalid` means the token in `credentials.h` is wrong or expired. HA long-lived tokens don't expire unless manually revoked.

---

## In-class member initializers break aggregate initialization

`AppState` is initialized as an aggregate in `AppState.cpp`:
```cpp
AppState appState = { .loveMode = true, .dirty = false, ... };
```

Adding a default-initializer to any `AppState` field (e.g. `bool loveMode = false;` in the header) converts the struct from an aggregate to a non-aggregate in C++14/17 — the designated initializer list then fails with "could not convert from brace-enclosed initializer list".

**Fix**: never add in-class initializers to `AppState` (or any struct initialized as an aggregate). Keep all defaults in the aggregate initializer in `AppState.cpp`.

---

## Use `lv_timer_create` for sub-60s card animation cycles

`RestCard::update()` is only called when `AppState` is dirty or every 60s from `RestScreen::tick()`. For animations that need to cycle faster (e.g. message rotation every 8s), use `lv_timer_create` inside `init()` — it fires via `lv_timer_handler()` independently of AppState.

```cpp
_msgTimer = lv_timer_create(msgTimerCb, MSG_INTERVAL_MS, this);
lv_timer_pause(_msgTimer);  // pause until show()

// In show():
lv_timer_reset(_msgTimer);
lv_timer_resume(_msgTimer);

// In hide():
lv_timer_pause(_msgTimer);
```

The timer callback gets `this` via `lv_timer_get_user_data(timer)`. For `lv_anim_t` callbacks (which are static), store `this` in the animated object's user_data with `lv_obj_set_user_data(obj, this)`.

---

## LVGL flex layout for centered cards

### Single row (icon + label pair)

Using `LV_ALIGN_LEFT_MID` / `LV_ALIGN_RIGHT_MID` on an icon and a text label inside a fixed-width container leaves a large gap because LVGL pins each child to the container edges regardless of content width. The gap varies by mode label length.

**Fix**: use flex layout so the pair sizes to content and centers itself:

```cpp
lv_obj_set_size(_container, LV_SIZE_CONTENT, 36);  // width shrinks to fit
lv_obj_set_style_pad_column(_container, 4, LV_PART_MAIN); // 4px gap between children
lv_obj_set_layout(_container, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
```

Children need no manual `lv_obj_align` — flex positions them. The container itself is then aligned on the parent with `lv_obj_align`.

### Full card layout (vertical stack of centered rows)

For cards with multiple rows of mixed-font content (e.g. big number + small unit, then icon + value + value, then icon + score), make the outer container a vertical flex column. Each row is a horizontal flex row sized to content. No manual `lv_obj_align` or `lv_obj_align_to` anywhere in `update()`.

```cpp
// Outer container — vertical flex, all rows centered
lv_obj_set_style_pad_row(_container, 8, LV_PART_MAIN); // row gap
lv_obj_set_layout(_container, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

// Per row — content-sized horizontal flex row
lv_obj_t* row = lv_obj_create(_container);
lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
lv_obj_set_style_pad_column(row, 6, LV_PART_MAIN);
lv_obj_set_layout(row, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
```

Row objects don't need to be stored as fields — they're owned by the parent and only the leaf labels need updating in `update()`. `update()` becomes purely text + color assignments with no layout calls.

### Two-column card layout

For cards with two side-by-side columns (e.g. ForecastCard today/tomorrow), use `makeRow` at the top level with a large `colGap`, then `makeColumn` for each column's vertical stack:

```cpp
lv_obj_t* cols = CardLayout::makeRow(_container, 30); // large gap between columns
lv_obj_t* left  = CardLayout::makeColumn(cols, CardLayout::PAD_ROW_LIST);
lv_obj_t* right = CardLayout::makeColumn(cols, CardLayout::PAD_ROW_LIST);
// children of left/right are auto-centered within their column
```

`makeColumn` is `makeRow` transposed: `LV_SIZE_CONTENT` size, transparent, `LV_FLEX_FLOW_COLUMN`, all-center align, `pad_row` instead of `pad_column`.

---

## LVGL Montserrat font only covers ASCII 0x20–0x7E

The built-in `lv_font_montserrat_*` fonts are compiled with a limited Unicode range. Characters outside ASCII render as squares:
- Em dash `—` (U+2014, UTF-8: `0xE2 0x80 0x94`) → square
- Cent sign `¢` (U+00A2) → square
- Euro sign `€` (U+20AC) → square

**Fix**: use ASCII-only placeholders and suffixes in placeholder text and formatted strings. E.g. use `"-"` not `"—"`, `"ct"` not `"¢"`, and avoid currency symbols in label text.

---

## SleepManager must check for window-end while sleeping

`tick()` originally returned immediately when `_sleeping` was true, so the display stayed off past 07:00 until the user manually interacted. The sleep window exit was never detected.

**Fix**: at the top of `tick()`, when sleeping, call `inSleepWindow()` — if false, auto-wake (restore brightness, play wake tone). Only then return. This way the first `tick()` after 07:00 wakes the display without any user input.

