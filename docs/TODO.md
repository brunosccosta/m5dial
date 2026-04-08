# TODO

Small improvements and known issues parked for later.

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

## Feature: Spotify playback control

Encoder-as-volume or play/pause/skip — but must not disrupt UI standardization. Best candidate: hook into the QuickPanel (swipe down) when Spotify is playing. QuickPanel already intercepts all input when visible, so Spotify controls there would be consistent and wouldn't change the RestScreen/MenuScreen contract.

Alternative: a MenuCard for Spotify (alongside AC, Heater). Less discoverable but cleaner separation.

---

## Feature: Energy card

New `RestCard` showing current power draw + daily consumption from HA energy monitoring. Needs `sensor.*` entities from HA energy dashboard. At-a-glance, no interaction needed — pure display card.

---

## Feature: Light / scene control

Originally started as lamp control but lamps not yet connected to HA. Two related threads:
- **Scene selection** (existing): activate HA scenes from the device — scene count, list vs. carousel UI TBD
- **Light control**: brightness/color temp slider for a room via `light.*` entity — encoder adjusts, button confirms; extremely natural on a dial

Connect lamps to HA first, then revisit both together.

---

## Feature: RFID

M5Dial has a built-in RFID reader (currently unused). Tap a card/fob → trigger HA action (arrive home, leave, arm alarm, profile switch). Requires: finding/buying compatible tags, investigating the M5Dial RFID API, designing the HA automation side. Not trivial — park until hardware is in hand.

---

## Explore: long press on encoder button

Button currently does the same thing (back / confirm) everywhere. Long press could be a dedicated shortcut — e.g. toggle a specific HA entity, go directly to a screen, or silence the buzzer. Low effort once a use case is clear.

---

## Feature: Hook FlightCard to HA calendar

`FlightCard` is built with flights hardcoded in `src/flights.h` (gitignored). Eventually drive it from a HA `calendar` entity or `input_datetime` helpers so flights can be added/removed from HA without reflashing.

---

