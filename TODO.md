# TODO

Small improvements and known issues parked for later.

---

## Buzzer feedback

M5Dial has an 80dB buzzer via `M5Dial.Speaker`. Potential uses: confirmation beep on button press, alert on sensor threshold (e.g. room too hot), error sound on WiFi/HA disconnect, encoder tick tone. Investigate what feels useful vs. annoying.

---

## Screen transition animations

Push/pop transitions (rest → menu, menu → control) are still bare `lv_scr_load()` calls. LVGL has `lv_scr_load_anim()` — could add a fade or slide. MenuScreen already has internal slide animation between cards; the same approach could extend to screen-level transitions.

---

## Feature: Easter egg screen — love messages controlled via HA

Add an `input_boolean.dial_love_mode` entity in HA. When enabled, show a rotating set of cute hardcoded messages on the RestScreen.

**Plan:**
- Subscribe to `input_boolean.dial_love_mode` in `HAClient`; store as `bool loveMode` in `AppState`
- Add a `LoveCard` rest card; `isVisible()` returns `appState.loveMode`
- Messages hardcoded as a small array in `LoveCard.cpp`, cycling on each `update()`

---
