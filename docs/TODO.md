# TODO

Small improvements and known issues parked for later.

---

## Initiative: UI standardization — rest cards + layout system

**Goal**: make all rest cards feel like one product. Every card uses a shared layout template; no card invents its own spacing, font sizes, or icon gaps.

### Step 1 — Define layout constants (unblocks everything else)

Add a `CardLayout` namespace (or extend `Theme.h`) with shared constants:
- Vertical offsets for hero, supporting rows
- Font roles: hero (48pt), value (28pt), detail (14pt)
- `PAD_ICON_TEXT` — gap between icon and its label (currently varies per card)
- `PAD_ROW` — vertical gap between rows in a flex column (currently 8px in EnergyCard — make this the standard)

EnergyCard is the reference implementation.

### Step 2 — Redesign QuickPanel + move heater there

**Current state**: QuickPanel has a thermometer row (indoor temp data) + a row with Find My shortcut.

**Target state**: remove the thermometer row. Single row with 3 actions + status items:
- Find My (existing)
- Heater status/shortcut (new — replaces heater rest card)
- TBD third slot (placeholder for future action)

Heater rest card gets deleted. QuickPanel becomes the canonical "quick actions" surface, not a mini status dashboard.

**What this gives**: one fewer rest card to swipe through; heater accessible from anywhere via swipe-down; QuickPanel has a cleaner single purpose.

### Step 1 status — ✓ done

- `CardLayout.h` created with shared constants, font roles, `makeContainer`, `makeRow`
- `EnergyCard` → Style A ✓
- `WeatherNowCard` → Style A ✓
- `IndoorTempsCard` → Style C ✓ (fixed-width columns, nested groups)
- Device strip removed; all cards own full screen ✓
- `SHIFT_UP=0` — all cards centered at true vertical midpoint ✓

### Step 3 — Refactor remaining cards to layout templates

Three templates:

| Style | Hero element | Cards |
|---|---|---|
| **A — Hero number** | Large number (48pt) + supporting rows | EnergyCard ✓, WeatherNowCard ✓, ClockCard |
| **B — Status + value** | Mode/state pill + arc | ACControlScreen (arc intentional, leave as-is) |
| **C — Equal-weight list** | 2–3 rows, same visual weight | IndoorTempsCard ✓, ForecastCard, MeshCoreCard |
| **Special** | Bespoke layout, keep as-is | SpotifyCard, LoveCard (animations), FlightCard (adopt makeContainer shell only) |

**Remaining card work:**
- `ClockCard` → Style A ✓
- `ForecastCard` → two `makeColumn`s inside `makeRow` ✓ (added `makeColumn` to CardLayout)
- `MeshCoreCard` → flex column with header/stats/status rows ✓
- `FlightCard` — adopt `makeContainer` outer shell; keep inner layout bespoke (dynamic centering of flag+destination pair is non-trivial)
- `HeaterCard` — **delete**, move to QuickPanel (Step 2)

### Priority order

1. ~~Layout constants (`CardLayout` namespace)~~ ✓
2. ClockCard, ForecastCard, MeshCoreCard (finish Step 3)
3. Heater → QuickPanel (Step 2)
4. FlightCard shell migration (low stakes)

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

## Feature: Energy card ✓

`EnergyCard` added as a `RestCard`. Subscribes to 5 Zonneplan sensors:
- `sensor.zonneplan_usage_kwh` — today's total kWh (big center number)
- `sensor.zonneplan_current_usage` — live watts (bolt icon row)
- `sensor.zonneplan_current_electricity_tariff` — tariff in €/kWh (same row)
- `sensor.zonneplan_sustainability_score` — 0–100, colored by value (leaf icon row)
- `sensor.zonneplan_status_tip` — Dutch status tip, scrolling at top, color-coded

Card visible only when `energy.valid`. Tip colors: green if "groen"/"goedkoop", orange if "hoog".

---

## Feature: Light / scene control

Originally started as lamp control but lamps not yet connected to HA. Two related threads:
- **Scene selection** (existing): activate HA scenes from the device — scene count, list vs. carousel UI TBD
- **Light control**: brightness/color temp slider for a room via `light.*` entity — encoder adjusts, button confirms; extremely natural on a dial

Connect lamps to HA first, then revisit both together.

---

## Feature: RFID — write speaker pucks

SpotifyClient done. Device IDs captured in `src/devices.h`. Remaining:
- Write `spotify:device:<id>` tags via NFC Tools for each speaker puck (Living Room AVR, TV, iPhone, Mac mini)
- Sonos parked — does not appear as Spotify Connect device when HA controls it

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

