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

## HA WebSocket auth flow

Fixed protocol on connect — no variation:
1. Server → `{"type":"auth_required","ha_version":"..."}`
2. Client → `{"type":"auth","access_token":"TOKEN"}`
3. Server → `{"type":"auth_ok"}` or `{"type":"auth_invalid"}`

`auth_invalid` means the token in `credentials.h` is wrong or expired. HA long-lived tokens don't expire unless manually revoked.
