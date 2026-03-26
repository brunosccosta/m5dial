# M5Stack Dial - Smart Home Controller

## Project Overview
A Home Assistant controller built on the M5Stack Dial (ESP32-S3) using LVGL for smooth animations and intuitive UI. The dial encoder and button provide navigation through a circular menu to control smart home devices.

## Hardware
- **Device**: M5Stack Dial (ESP32-S3)
- **Display**: 1.28" round TFT, 240×240, GC9A01 driver
- **Input**: Rotary encoder + center button
- **Connectivity**: WiFi
- **Extras**: Built-in haptic motor, RTC, microphone

## UI Design

### Circular Menu
- **Center**: Selected item shown large and prominently
- **Border**: Remaining items shown small, arranged around the ring edge
- Rotating the dial shifts which item is centered; the ring animates accordingly
- Pressing the button enters the selected item's control screen

### Control Screens
Each device type has its own full-screen control interface, navigated by dial + button.

### Visual Style
- Dark background (black)
- High contrast white text for selected item
- Dimmed text for border items
- Smooth LVGL animations on transitions

## Tech Stack
- **Platform**: PlatformIO, ESP32-S3, Arduino framework
- **UI**: LVGL v9 — pure C++, no XML editor
- **Display driver**: M5GFX via M5Unified
- **HA communication**: WebSocket API (MQTT as fallback if needed — decide at Milestone 3)
- **Credentials**: Hardcoded (WiFi SSID/password, HA token, HA host)

## Target Entities
- 2× Air conditioners (temperature, mode)
- N× Lamps (on/off, brightness)
- Temperature sensor display (optional, nice-to-have)

## Development Milestones

### Milestone 1: Hardware Validation ✅
**Goal**: Display works, input detected

- [x] Initialize M5Stack Dial hardware
- [x] Setup LVGL v9 with GC9A01 via M5GFX
- [x] Render something on screen to confirm the pipeline works
- [x] Rotary encoder input read correctly
- [x] Center button input detected
- **Deliverable**: Screen shows content, encoder and button respond

### Milestone 2: Circular Menu
**Goal**: Core navigation working

- [ ] Circular menu layout: selected item in center (large), others on border ring (small)
- [ ] Dial rotation cycles through items with animation
- [ ] Button press enters selected item
- [ ] Back navigation (long press or dedicated behavior — TBD)
- [ ] Menu items: Lamps, Air Conditioners, Temperature (placeholder screens)
- **Deliverable**: Navigable circular menu

### Milestone 3: WiFi & Home Assistant
**Goal**: Connected to HA, reading real state

- [ ] WiFi connect with hardcoded credentials
- [ ] HA WebSocket connection + authentication
- [ ] Subscribe to entity state updates (lamps, ACs, temp sensor)
- [ ] Display real entity states in menu / control screens
- [ ] Basic error handling (no WiFi, HA unreachable)
- **Deliverable**: Live state from HA visible on device

### Milestone 4: Lamp Control
**Goal**: Full lamp control via HA

- [ ] Lamp list screen (if multiple lamps)
- [ ] On/off toggle
- [ ] Brightness control via dial
- [ ] Show current state (on/off, brightness level)
- **Deliverable**: Lamps fully controllable from device

### Milestone 5: AC Control
**Goal**: Full aircon control via HA

- [ ] AC control screen for each unit
- [ ] Temperature setpoint via dial
- [ ] Mode selection (cool, heat, auto, off)
- [ ] Show current temp + target + mode
- **Deliverable**: Both ACs fully controllable

### Milestone 6: Polish
**Goal**: Feels good to use

- [ ] Haptic feedback on scroll and selection
- [ ] Screensaver / sleep after inactivity
- [ ] Smooth animations and transitions
- [ ] Temperature sensor display (if available)
- **Deliverable**: Production-quality feel

## Logging

Uses ESP-IDF's `esp_log` (`#include <esp_log.h>`). Controlled by `CORE_DEBUG_LEVEL` in `platformio.ini`.

### Levels
| Level | Macro | When to use |
|---|---|---|
| 0 | — | Silent (default / production) |
| 1 | `ESP_LOGE` | Errors: crashes, unrecoverable failures |
| 2 | `ESP_LOGW` | Warnings: unexpected but recoverable states |
| 3 | `ESP_LOGI` | Info: lifecycle events (boot, screen load, WiFi connect) |
| 4 | `ESP_LOGD` | Debug: user interactions (encoder, button, menu nav) |
| 5 | `ESP_LOGV` | Verbose: per-frame events (LVGL flush, timer ticks) |

Default: `CORE_DEBUG_LEVEL=1` (errors only). Set to `4` to debug interactions, `5` for render pipeline.

### Tags
| Tag | File | Covers |
|---|---|---|
| `BOOT` | main.cpp | Initialization sequence |
| `INPUT` | main.cpp | Encoder and button events |
| `MENU` | CarouselMenu.cpp | Menu scroll, selection, state |
| `DISPLAY` | main.cpp | LVGL flush / render events |
| `WIFI` | *(future)* | WiFi connection lifecycle |
| `HA` | *(future)* | Home Assistant communication |

### Message format
- Lowercase, no trailing punctuation
- Data as `key=value` pairs inline: `ESP_LOGD("MENU", "scroll index=%d label=%s", idx, label)`
- No redundant tag info in the message (the tag already says the component)

## Open Questions
- Back navigation: long press center button, or timeout back to menu?
- Multiple lamps: flat list or grouped? How many?
- AC modes supported: depends on HA climate entity config
- Temperature sensor: which entity / where displayed?

---
*Last updated: March 2026*
