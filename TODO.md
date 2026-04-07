# TODO

Small improvements and known issues parked for later.

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

## Tooling: LVGL image pipeline

Color images (emoji, icons) can't be embedded in fonts — they need to be compiled C arrays (`lv_img_dsc_t`) and displayed via `lv_img_create()`. Set up a pipeline similar to the font tooling:

- `tools/images/` — source PNGs (e.g. Twemoji peach `1F351.png`, flag PNGs)
- `tools/gen_images.sh` — converts each PNG to an LVGL C array using `lv_img_converter` (or the Python equivalent from LVGL's repo)
- `src/ui/images/` — generated `.c` files + a `lvgl_images.h` header declaring all `lv_img_dsc_t` externs

First use case: peach emoji for `LoveCard` ("Gostosa!"), flag emojis for the flight countdown card.

---

## Feature: Flight/vacation countdown card

A rest card showing a countdown to the next upcoming flight or vacation. Could show: destination name, flag emoji, days remaining, maybe departure city → destination city. Data hardcoded initially (or driven by a HA `input_datetime` / `calendar` entity later).

**Open questions:**
- Single next event or a short list (next 2–3)?
- Flag emoji → requires color emoji image rendering (see peach emoji task)
- Data source: hardcoded array vs. HA calendar entity subscription

---

## Feature: Hook LoveCard to HA input_boolean

`LoveCard` is built and `appState.loveMode` is hardcoded `true`. Wire it up to HA:
- Add `input_boolean.dial_love_mode` entity in HA
- Subscribe in `HAClient`; write to `appState.loveMode` on state change
- `LoveCard::isVisible()` already returns `appState.loveMode` — no card changes needed

---
