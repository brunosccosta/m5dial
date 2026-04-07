# M5Dial SmartHome

A Home Assistant dashboard running on the [M5Dial](https://docs.m5stack.com/en/core/M5Dial) (ESP32-S3, 240×240 circular GC9A01 display, rotary encoder, touch).

Displays at-a-glance home status — weather, indoor temps, Spotify, AC/heater controls, flight countdowns — and lets you control devices from the dial.

---

## Setup

### 1. Dependencies

Install [PlatformIO](https://platformio.org). All libraries are declared in `platformio.ini` and fetched automatically on first build.

### 2. Credentials

Copy and fill in:

```bash
cp src/credentials.h.example src/credentials.h   # WiFi + HA token
cp src/flights.h.example src/flights.h            # upcoming flights
```

`src/credentials.h` and `src/flights.h` are gitignored.

### 3. Build and flash

```bash
~/.platformio/penv/bin/pio run --target upload
```

### 4. Monitor logs

```bash
~/.platformio/penv/bin/pio device monitor
```

---

## Tooling

### Fonts

FontAwesome Solid glyphs + Cyrillic Montserrat, compiled to LVGL C arrays.

```bash
bash tools/gen_fonts.sh
```

→ [`src/ui/fonts/README.md`](src/ui/fonts/README.md)

### Images (color emoji, flags)

Twemoji PNGs compiled to LVGL `lv_image_dsc_t` C arrays.

```bash
bash tools/setup_venv.sh   # once
bash tools/gen_images.sh
```

→ [`src/ui/images/README.md`](src/ui/images/README.md)

---

## Docs

| Doc | What's in it |
|---|---|
| [`docs/TODO.md`](docs/TODO.md) | Planned features and open questions |
| [`docs/architecture.md`](docs/architecture.md) | System overview — AppState, HAClient, screens, cards, input contract, loop structure |
| [`docs/learnings.md`](docs/learnings.md) | LVGL/ESP32 gotchas and non-obvious fixes |
| [`docs/theme-system.md`](docs/theme-system.md) | Color tokens and how to add a new theme |
| [`docs/spotify-sonos.md`](docs/spotify-sonos.md) | Spotify + Sonos integration details |
| [`src/ui/fonts/README.md`](src/ui/fonts/README.md) | How to add FA glyphs and regenerate fonts |
| [`src/ui/images/README.md`](src/ui/images/README.md) | How to add color emoji/flag images |
