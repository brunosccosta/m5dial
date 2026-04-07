# TODO

Small improvements and known issues parked for later.

---

## Feature: Sleep mode (00–07h display off)

Between 00:00 and 07:00, turn the display off after 60s of inactivity. Any input wakes it; the wake event is swallowed (doesn't propagate to the screen).

**Design:**
- `SleepManager` class (`src/SleepManager.h/.cpp`) — tracks last activity, manages brightness
- `sleepManager.onActivity()` — resets timer; if sleeping, restores brightness and clears `_sleeping`
- `sleepManager.tick()` — if in sleep window + idle > `SLEEP_TIMEOUT_MS` → set brightness to `SLEEP_BRIGHTNESS` and set `_sleeping`
- `sleepManager.isSleeping()` — checked in `main.cpp` before routing input; if true, call `onActivity()` and skip all further input handling
- Sleep window: `tm_hour >= 0 && tm_hour < 7` — guard against un-synced clock (time < 2020 → skip)
- Outside 00–07: display always on (no idle timeout for now)

**Constants (in `SleepManager.h`):**
- `SLEEP_BRIGHTNESS = 0` — fully off; make it a param to `begin()` for easy testing
- `SLEEP_TIMEOUT_MS = 60000` — 60s
- `SLEEP_HOUR_START = 0`, `SLEEP_HOUR_END = 7`

**Integration in `main.cpp`:**
- `sleepManager.begin(128)` in `setup()` (pass normal brightness so it can restore it)
- `sleepManager.onActivity()` on every encoder delta, button press, touch event
- `sleepManager.tick()` at the end of `loop()`
- If `sleepManager.isSleeping()`: swallow all input for that loop iteration

---

## Buzzer feedback

M5Dial has an 80dB buzzer via `M5Dial.Speaker`. Potential uses: confirmation beep on button press, alert on sensor threshold (e.g. room too hot), error sound on WiFi/HA disconnect, encoder tick tone. Investigate what feels useful vs. annoying.

---

## Screen transition animations

Push/pop transitions (rest → menu, menu → control) are still bare `lv_scr_load()` calls. LVGL has `lv_scr_load_anim()` — could add a fade or slide. MenuScreen already has internal slide animation between cards; the same approach could extend to screen-level transitions.

---

## Feature: Notification system

Use HA persistent notifications as the backend. Any HA automation pushes `persistent_notification.create` (title + message + notification_id); the dial subscribes to those entities, shows a badge on RestScreen, and can dismiss via `persistent_notification.dismiss`.

**Full loop:**
```
HA automation → persistent_notification.create → HAClient sees entity change
  → AppState.notifications[] → dot on RestScreen + MenuCard list
  → user dismisses on dial → persistent_notification.dismiss → HA clears it
```

**Open questions:**
- **Dot position**: not `~1 o'clock` (taken by error overlay) — pick a distinct position
- **Persistence**: treat as unread until explicitly dismissed; no auto-expiry
- **Menu card**: shows unread count in label; `onSelect()` pushes a new `NotificationListScreen` (scroll + dismiss per item)
- **AppState**: add `NotificationEntry[]` array (title, message, id, unread flag); max capacity TBD (8–16 entries?)

---

## Feature: Scene selection

Allow activating HA scenes from the device. Rest card could show the current/last-activated scene. Details TBD — scene count, list vs. carousel UI, how HA surfaces active scene as an entity (or just track last-activated locally).

---

## Feature: Hook FlightCard to HA calendar

`FlightCard` is built with flights hardcoded in `src/flights.h` (gitignored). Eventually drive it from a HA `calendar` entity or `input_datetime` helpers so flights can be added/removed from HA without reflashing.

---

## Feature: Hook LoveCard to HA input_boolean

`LoveCard` is built and `appState.loveMode` is hardcoded `true`. Wire it up to HA:
- Add `input_boolean.dial_love_mode` entity in HA
- Subscribe in `HAClient`; write to `appState.loveMode` on state change
- `LoveCard::isVisible()` already returns `appState.loveMode` — no card changes needed

---
