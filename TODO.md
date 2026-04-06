# TODO

Small improvements and known issues parked for later.

---

## Buzzer feedback

M5Dial has an 80dB buzzer via `M5Dial.Speaker`. Potential uses: confirmation beep on button press, alert on sensor threshold (e.g. room too hot), error sound on WiFi/HA disconnect, encoder tick tone. Investigate what feels useful vs. annoying.

---

## Screen transition animations

Push/pop transitions (rest → menu, menu → control) are still bare `lv_scr_load()` calls. LVGL has `lv_scr_load_anim()` — could add a fade or slide. MenuScreen already has internal slide animation between cards; the same approach could extend to screen-level transitions.

---

## ~~Feature: Find My iPhone~~ (done)

Trigger `icloud.play_sound` via HA — rings the phone even through mute/DND. Requires HA iCloud integration with account email + device name parameters.

**Plan:**
- Add `haClient.sendFindMyIPhone()` → `call_service: icloud.play_sound` with account + device_name from `credentials.h`
- Add `MenuCardFindMy` — label "Find iPhone", FA icon `FA_LOCATION_DOT` or similar; `onSelect()` calls the above then pops back
- No confirm screen needed (action is harmless and reversible by the user picking up the phone)
- If quick-action cards accumulate, give them a dedicated menu sub-section

---

## Feature: MeshCore repeater rest card

I have a MeshCore repeater integrated in HA. Add a `RestCard` showing basic stats from it — exact fields TBD (candidates: node count, last seen, SNR/RSSI, uptime). Pull via existing HA entity subscription. Card is always visible.

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

## Feature: Easter egg screen — love messages controlled via HA

Add an `input_boolean.dial_love_mode` entity in HA. When enabled, show a rotating set of cute hardcoded messages on the RestScreen.

**Plan:**
- Subscribe to `input_boolean.dial_love_mode` in `HAClient`; store as `bool loveMode` in `AppState`
- Add a `LoveCard` rest card; `isVisible()` returns `appState.loveMode`
- Messages hardcoded as a small array in `LoveCard.cpp`, cycling on each `update()`

---
