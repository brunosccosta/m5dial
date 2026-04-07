# M5Stack Dial - Smart Home Controller

## Project Overview
A Home Assistant controller built on the M5Stack Dial (ESP32-S3) using LVGL for smooth animations. The dial encoder and button navigate a circular menu to control smart home devices.

## Hardware
- **Device**: M5Stack Dial (ESP32-S3)
- **Display**: 1.28" round TFT, 240×240, GC9A01 driver
- **Input**: Rotary encoder + center button
- **Connectivity**: WiFi
- **Extras**: Buzzer, RFID (WS1850S), RTC (BM8563), touch (FT3267)

## Target Entities
- 2× Air conditioners (temperature, mode)
- N× Lamps (on/off, brightness)
- 3× Temperature + humidity sensors (balcony, bedroom, bathroom)

## Docs

### Project docs
- [Architecture & design decisions](docs/architecture.md)
- [Learnings & gotchas](docs/learnings.md)
- [Open tasks & future work](docs/TODO.md)

### Source READMEs
- [RestScreen cards](src/ui/cards/README.md) — how to add a new card
- [Font icons](src/ui/fonts/README.md) — how to add a FontAwesome glyph
- [Themes](src/ui/themes/README.md) — color token reference, how to add a theme

---

## Common tasks

### Adding a RestScreen card
Cards are the rotating info panels on the idle screen. See [`src/ui/cards/README.md`](src/ui/cards/README.md) — 3 steps: new `.h`/`.cpp` file, add member to `RestScreen.h`, register in `RestScreen::init()`.

### Adding a new icon
Icons are FontAwesome glyphs compiled into LVGL bitmap fonts. See [`src/ui/fonts/README.md`](src/ui/fonts/README.md) — find the codepoint, regenerate all three font files (32px + 24px + 18px), add the `#define` macro to `fa_icons.h`.

### Changing UI colors / adding a theme
All colors are named constants. See [`src/ui/themes/README.md`](src/ui/themes/README.md) — copy `ThemeDefault.h`, edit the values, register in `Theme.h`, set the build flag.

---
*Last updated: April 2026*
