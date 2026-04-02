# Spotify + Sonos Control — Research Notes

## Entities

| Entity | Type | Notes |
|---|---|---|
| `media_player.spotify` | Spotify HA integration | Controls Spotify playback |
| `media_player.sala` | Sonos (via Sonos integration) | Living room speaker |
| `media_player.living_room` | Denon/Marantz AV receiver (HEOS) | Spotify Connect capable via HEOS. Spotify Connect name: **"Living Room"**. Retains full control (`supported_features: 444983`) — controls stay on `media_player.spotify`, unlike Sala. |

---

## What HA sends for media_player.spotify

Full field list confirmed via `subscribe_entities` snapshot:

| Field | Type | Example | Notes |
|---|---|---|---|
| state | string | `"playing"` | `"playing"` / `"paused"` / `"idle"` / `"off"` |
| `source` | string | `"iPhone"` / `"Sala"` | Active playback device |
| `source_list` | array | `["iPhone"]` | **Dynamic** — only present when Spotify can see available devices. Removed via `-` diff when unavailable. |
| `media_title` | string | `"Too Sweet"` | — |
| `media_artist` | string | `"Hozier"` | — |
| `media_album_name` | string | `"Unreal Unearth: Unaired"` | — |
| `media_duration` | int | `251` | seconds |
| `media_position` | int | `149` | seconds; stale — interpolate using `media_position_updated_at` |
| `media_position_updated_at` | string | `"2026-04-02T19:26:17+00:00"` | UTC ISO8601 |
| `shuffle` | bool | `false` | — |
| `repeat` | string | `"off"` | `"off"` / `"one"` / `"all"` |
| `volume_level` | float | `1.0` | 0.0–1.0 |
| `entity_picture` | string | `/api/media_player_proxy/...` | HA-proxied album art — requires HTTP fetch + JPEG decode to render |
| `supported_features` | int | `444983` / `2048` | **Changes depending on playback device** (see below) |

### supported_features — decoded

When playing on **iPhone** (`444983`): full control — play/pause, seek, next/prev, volume, select_source, shuffle, repeat.

When playing on **Living Room/HEOS** (`444983`): same full control — Spotify retains all features. `media_content_id` is a real `spotify:track:` URI.

When playing on **Sala/Sonos** (`2048`): only `SELECT_SOURCE`. Spotify hands off control to the Sonos — play/pause/next must go through `media_player.sala` instead.

---

## What HA sends for media_player.sala

| Field | Type | Example | Notes |
|---|---|---|---|
| state | string | `"playing"` / `"idle"` | — |
| `source` | string | `"Spotify Connect"` | When receiving Spotify audio |
| `volume_level` | float | `0.32` | 0.0–1.0 |
| `is_volume_muted` | bool | `false` | — |
| `media_title` | string | `"Someone New"` | Mirrored from Spotify |
| `media_artist` | string | `"Hozier"` | — |
| `media_album_name` | string | `"Hozier"` | — |
| `media_duration` | int | `222` | seconds |
| `media_position` | int | `26` | seconds |
| `group_members` | array | `["media_player.sala"]` | Sonos grouping — only sala for now |
| `supported_features` | int | `4127295` | Full Sonos feature set including GROUPING |
| `media_content_id` | string | `x-sonos-vli:RINCON_347E5CFA409201400:2,spotify:...` | Sonos-internal URI format, not a standard Spotify URI |

---

## Casting — what works and what doesn't

### `media_player.play_media` on `media_player.sala` with `spotify:track:xxx`
**Result: UPnP Error 800 (failed).** Sonos doesn't accept raw Spotify URIs via UPnP directly.

### `media_player.select_source` on `media_player.spotify` with `source: "Sala"`
**Result: not confirmed yet.** First attempt failed — but at that point music was already playing on Sala (user had manually cast from iPhone before the test). The Sonos may need to be active/visible to Spotify before `select_source` works.

### Manual cast from Spotify app → Sonos
**Works.** When the user casts from the Spotify app on iPhone, `media_player.spotify.source` becomes `"Sala"` and `media_player.sala.source` becomes `"Spotify Connect"`. Both entities update via `subscribe_entities`.

---

## Key behaviours

- **`source_list` is dynamic.** It appears when Spotify can enumerate Connect devices and is explicitly removed (`-` diff) when it can't. Don't rely on it being present.
- **Controls shift between entities only for Sonos.** iPhone and Living Room (HEOS): always control via `media_player.spotify` (`supported_features: 444983`). Sonos only: control via `media_player.sala` (`supported_features: 2048` — Spotify loses most features).
- **`media_position` is stale.** HA only sends diffs every ~30s. Use `media_position_updated_at` + elapsed time to interpolate current position for a progress bar.
- **Sonos device name in Spotify Connect is "Sala"** — confirmed by `media_player.spotify.source = "Sala"` when casting.

---

## Open questions

1. Does `select_source` on `media_player.spotify` with `source: "Sala"` work when initiated cold (Sonos idle)? Untested.
2. Can we wake the Sonos first (`media_player.turn_on`) and then `select_source`?
3. For a control screen: should controls route to `media_player.spotify` or `media_player.sala` depending on active source? Or always prefer `media_player.sala` when it's active?

---

## Planned control screen interactions

**Rule**: use `media_player.sala` for controls only when `media_player.spotify.source == "Sala"`. All other sources → `media_player.spotify`.

| Action | Default (iPhone / Living Room / other) | When on Sala |
|---|---|---|
| Play / pause | `media_player.media_play_pause` → spotify | `media_player.media_play_pause` → sala |
| Next / prev | `media_player.media_next_track` → spotify | `media_player.media_next_track` → sala |
| Volume | `media_player.volume_set` → spotify | `media_player.volume_set` → sala |
| Cast to Sala | `media_player.select_source` → spotify, `source: "Sala"` | already there |
| Cast to Living Room | `media_player.select_source` → spotify, `source: "Living Room"` | `media_player.select_source` → spotify, `source: "Living Room"` |
| Cast to iPhone | `media_player.select_source` → spotify, `source: "iPhone"` | `media_player.select_source` → spotify, `source: "iPhone"` |
