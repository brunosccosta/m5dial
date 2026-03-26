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

## WebSockets library (Links2004)

Not yet integrated — noted here for when Milestone B1 begins:
- PlatformIO lib id: `links2004/WebSockets`
- Non-blocking usage: call `webSocket.loop()` every Arduino loop tick
- HA WebSocket API requires a specific handshake + auth_required flow before subscribing to entities
