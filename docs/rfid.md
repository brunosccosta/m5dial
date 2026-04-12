# RFID Feature — Working Doc

## Goal

Physical Spotify puck collection. Tap a tag → play playlist/album or transfer playback to a room.

## Hardware

- **Reader**: WS1850S / MFRC522, I2C, accessed via `M5Dial.Rfid`
- **Tags**: NTAG213, 20–25mm (144 bytes user memory, MIFARE Ultralight-compatible)
- **Form factor**: 3D-printed pucks — dark body + translucent top + paper album art insert

## Two puck types (same tag format, routed by URI prefix)

| Puck | Tag content | Action |
|---|---|---|
| Playlist puck | `spotify:playlist:...` / `spotify:album:...` | `media_player.play_media` on current speaker |
| Speaker puck | `ha:media_player.sala` / `ha:media_player.quarto` | `media_player.transfer` — moves playback to that room |

Tags written from phone via NFC Tools as plain NDEF Text records.

---

## Tag writing

Tags are written from phone via NFC Tools (iOS/Android) as plain NDEF Text records.

**String format:**
- Playlist puck: `spotify:playlist:<22-char base62 id>`
- Album puck: `spotify:album:<22-char base62 id>`
- Speaker puck: `ha:media_player.sala` / `ha:media_player.quarto`

**Tooling (TODO):** `tools/gen_tag.py` — takes human-friendly input, outputs exact string to paste into NFC Tools, validates ID length and speaker names.

```
python tools/gen_tag.py playlist 37i9dQZF1DXcQ7lvApgNa9
→ spotify:playlist:37i9dQZF1DXcQ7lvApgNa9

python tools/gen_tag.py speaker sala
→ ha:media_player.sala
```

**iPhone + MIFARE Classic**: Apple's NFC stack does not support MIFARE Classic (no CRYPTO1 license). NFC Tools on iOS cannot detect or write Classic tags at all. NTAG213 (MIFARE Ultralight) works fine on iPhone — another reason it's the right tag choice.

---

## Architecture

Three layers, fully decoupled:

```
RFIDReader          → hardware only: NDEF parse, debounce, fires callback(uri)
RFIDDispatcher      → prefix match, routes uri to registered RFIDHandler
RFIDHandler impls   → per-type logic (SpotifyHandler, HAHandler, ...)
```

Adding a new tag type = new handler file + one `registerHandler()` call. Nothing else changes.

### RFIDHandler interface (`src/RFIDDispatcher.h`)
```cpp
class RFIDHandler {
public:
    virtual const char* prefix() const = 0;
    virtual void handle(const char* uri) = 0;
};
```

### Registered handlers

| Handler | Prefix | Action |
|---|---|---|
| `SpotifyHandler` | `spotify:` | `haClient.sendPlayMedia("media_player.spotify", uri)` |
| `HAHandler` | `ha:` | strips prefix → `haClient.sendTransferMedia(entity_id)` |

**TODO:** `sendPlayMedia` hardcodes `media_player.spotify` as target. Revisit when exploring active-speaker routing — several corner cases (e.g. playback already on a room speaker, no active session).

### main.cpp wiring
```cpp
rfidDispatcher.registerHandler(&spotifyHandler);
rfidDispatcher.registerHandler(&haHandler);
rfidReader.onTag([](const char* uri) { rfidDispatcher.dispatch(uri); });
```

---

## Implementation steps

### Step 1 — Raw tag dump ✓ VALIDATED (2026-04-08)
Enable RFID, detect any tag, log UID + type + raw page bytes to Serial.

**Findings:**
- RFID hardware works. Tag detected, UID read correctly.
- Home test tag was MIFARE Classic 1K — irrelevant to project.
- NTAG213 confirmed as `MIFARE Ultralight` — no auth needed, `MIFARE_Read` works directly.

### Step 2 — NDEF layout ✓ VALIDATED (2026-04-12)
NTAG213 written via NFC Tools (Text record, `spotify:album:2WWakvH7foDm8RjWFMDcL7`).

Confirmed layout (pages 4–11):
```
page 04: 03 2B D1 01   → TLV NDEF (43 bytes), record header MB|ME|SR, type_len=1
page 05: 27 54 02 65   → payload_len=39, type='T', status=0x02 (lang_len=2 UTF-8), lang[0]='e'
page 06: 6E 73 70 6F   → lang[1]='n', text="spo"...
```
TLV parse: `0x03` tag → 1-byte length → NDEF record → Text type → skip lang → extract payload.
No external library needed.

### Step 3 — Full implementation ✓ DONE (2026-04-12)
- `RFIDReader`: real NDEF TLV parser + 2s UID debounce; Classic support removed
- `RFIDDispatcher` + `RFIDHandler` interface: `src/RFIDDispatcher.h/.cpp`
- `SpotifyHandler`: `src/SpotifyHandler.h/.cpp` — `spotify:` prefix → `sendPlayMedia`
- `HAHandler`: `src/HAHandler.h/.cpp` — `ha:` prefix → `sendTransferMedia`
- `HAClient`: `sendPlayMedia`, `sendTransferMedia` added
- `main.cpp`: dispatcher wired; debug callback removed

**Finding:** `media_player.spotify` is a read/tracking entity — it does not accept `play_media`. Spotify playback must go through the Spotify Web API directly.

### Step 4 — Spotify Web API integration ← next

Replace `haClient.sendPlayMedia` with direct Spotify API calls from a new `SpotifyClient` class.

**Why API over HA:** `play_media` on `media_player.spotify` is rejected by HA ("entity does not support action"). The Spotify Web API targets the currently active device directly — no need to know which speaker is active, works with phone+Bluetooth.

**Auth setup (one-time, on computer):**
1. Create app at developer.spotify.com → get `client_id` + `client_secret`
2. Authorization Code flow:
   - GET `https://accounts.spotify.com/authorize?client_id=...&response_type=code&redirect_uri=...&scope=user-modify-playback-state%20user-read-playback-state`
   - Exchange code → POST `https://accounts.spotify.com/api/token`
   - Store `refresh_token` in `credentials.h`
3. M5Dial at boot: POST refresh_token → access_token (1h TTL), refresh before expiry

**Token rotation:** Spotify may issue a new refresh token alongside the access token. Store updated refresh token in ESP32 NVS so it survives reboots without reflashing. Start with credentials.h; add NVS persistence as follow-up.

**API calls:**
- **Play** (playlist/album puck):
  `PUT https://api.spotify.com/v1/me/player/play`
  Body: `{"context_uri":"spotify:album:..."}` — plays on currently active device
- **Get devices** (for speaker puck setup):
  `GET https://api.spotify.com/v1/me/player/devices` — returns id, name, type, is_active
- **Transfer** (speaker puck):
  `PUT https://api.spotify.com/v1/me/player`
  Body: `{"device_ids":["<spotify_device_id>"],"play":true}`

**Speaker puck format:** tag stores `ha:media_player.sala` today (routing to HA). After this step, speaker pucks will store Spotify device IDs or friendly names and be routed differently. Format TBD — may keep `ha:` prefix for HA-native speakers (Sonos via HA) and add `spotify:device:` prefix for pure Spotify Connect.

**New class:** `src/SpotifyClient.h/.cpp`
- `begin()` — reads credentials, triggers first token refresh
- `update()` — checks token expiry, refreshes if <60s remaining
- `play(const char* context_uri)` — PUT /me/player/play
- `transfer(const char* device_id)` — PUT /me/player
- `getDevices(...)` — GET /me/player/devices (for setup/debugging)

`SpotifyHandler` switches from `haClient.sendPlayMedia` to `spotifyClient.play(uri)`.

---

## Key API notes

**Enable**: `M5Dial.begin(cfg, true, true)` (encoder=true, RFID=true)

**Poll**:
```cpp
if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
    // uid in M5Dial.Rfid.uid
    M5Dial.Rfid.PICC_HaltA();
    // NO PCD_StopCrypto1 — that's MIFARE Classic only
}
```

**Read pages** (NTAG213 user memory starts at page 4, 4 bytes/page):
```cpp
uint8_t buf[18]; uint8_t size = 18;
M5Dial.Rfid.MIFARE_Read(pageNum, buf, &size); // reads 4 pages at once (16 bytes)
```

**Type check**: expect `MFRC522::PICC_TYPE_MIFARE_UL` for NTAG213.
MIFARE Classic tags (from official example) require key auth — skip those.

**NDEF TLV** (page 4 onward):
```
0x03 <len> [NDEF record bytes] 0xFE
```
NDEF Text record: header byte | type length | payload length | type "T" | lang length+lang | text payload
