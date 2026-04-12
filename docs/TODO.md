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

## Feature: RFID — Spotify Web API playback ← next

RFID hardware + dispatcher pipeline is done (see `docs/rfid.md`). Remaining work:

- Build `SpotifyClient` (`src/SpotifyClient.h/.cpp`) — OAuth token refresh + Spotify Web API calls
- `play(context_uri)` → `PUT /me/player/play` — targets active device, works with phone+BT
- `transfer(device_id)` → `PUT /me/player` — move playback to a Spotify Connect device
- Token rotation: persist refresh token to NVS so it survives power cycles without reflashing
- Wire `SpotifyHandler` to `SpotifyClient::play()` instead of `haClient.sendPlayMedia()`
- Speaker puck format: decide between `ha:` (HA-native, e.g. Sonos via HA) vs `spotify:device:` (pure Spotify Connect) — may support both

One-time setup: Spotify developer app (client_id + client_secret) + Authorization Code flow on computer to get initial refresh token → store in `credentials.h`.

---

## Feature: RFID — Spotify tag collection (original spec, archived)

M5Dial has a built-in RFID reader (WS1850S, ISO 14443A). Primary use case: physical Spotify tokens — tap a tag to play a playlist/album on HA media player.

**How it works:**
- Each tag stores a Spotify URI as a plain NDEF text record (e.g. `spotify:playlist:37i9dQZF1DX...`) written from phone via NFC Tools app
- M5Dial reads the URI, calls `media_player.play_media` via HAClient — no hardcoded UIDs, tag is self-describing
- Adding a new playlist = new tag + NFC Tools on phone, zero reflashing

**Hardware:**
- Tags: **NTAG213, 20–25mm** — 144 bytes user memory, more than enough for a Spotify URI (~50 bytes with NDEF overhead). Ordered/in hand.
- Form factor: **3D-printed pucks** (30–35mm diameter, 5–6mm thick), tag embedded in body with thin top layer (≤2mm) for reliable reads. Displayed in a small tray/rack near the dial — a physical collection, one per playlist.
- Puck design: dark filament body + translucent top layer + printed album art paper insert between them. Debossed artist/album name optional.

**Two puck types, same tag format — distinguished by URI prefix:**

| Puck | Tag content | Action |
|---|---|---|
| Playlist puck | `spotify:playlist:...` / `spotify:album:...` | `media_player.play_media` on current speaker |
| Speaker puck | `ha:media_player.sala` / `ha:media_player.quarto` | `media_player.transfer` — moves current playback to that speaker |

Speaker pucks are shaped/labeled to represent the room (e.g. speaker silhouette). Sit in the same tray as playlist pucks. Tap a speaker puck to move music to that room without touching a phone.

**Implementation notes:**
- Investigate WS1850S API (likely via M5Dial library)
- Keep top layer ≤3mm above tag for reliable read range; test single-wall print first
- Tag must be flat/parallel to dial face — reads face-to-face, not edge-on
- URI prefix routing: `spotify:` → play_media, `ha:media_player.*` → transfer
- Add `haClient.sendPlayMedia(entity_id, uri)` and `haClient.sendTransferMedia(target_entity)` methods
- Show brief toast on successful tap (what's playing / where it moved)

---

## Explore: long press on encoder button

Button currently does the same thing (back / confirm) everywhere. Long press could be a dedicated shortcut — e.g. toggle a specific HA entity, go directly to a screen, or silence the buzzer. Low effort once a use case is clear.

---

## Feature: Hook FlightCard to HA calendar

`FlightCard` is built with flights hardcoded in `src/flights.h` (gitignored). Eventually drive it from a HA `calendar` entity or `input_datetime` helpers so flights can be added/removed from HA without reflashing.

---

