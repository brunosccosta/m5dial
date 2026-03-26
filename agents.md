# M5Stack Dial - Smart Home Controller

## Project Overview
A Home Assistant controller built on the M5Stack Dial (ESP32-S3) using LVGL for smooth animations. The dial encoder and button navigate a circular menu to control smart home devices.

## Hardware
- **Device**: M5Stack Dial (ESP32-S3)
- **Display**: 1.28" round TFT, 240×240, GC9A01 driver
- **Input**: Rotary encoder + center button
- **Connectivity**: WiFi
- **Extras**: Built-in haptic motor, RTC, microphone

## Target Entities
- 2× Air conditioners (temperature, mode)
- N× Lamps (on/off, brightness)
- Temperature sensor display (optional)

## Docs
- [Architecture & design decisions](docs/architecture.md)
- [Learnings & gotchas](docs/learnings.md)

---

## Milestones

### Milestone 1: Hardware Validation ✅
Display works, encoder and button respond.

---

### Backend Lane

#### B1: WiFi + HA Connection
- [ ] Connect to WiFi with hardcoded credentials
- [ ] Open HA WebSocket connection
- [ ] Authenticate with long-lived token
- [ ] Confirm connection with a heartbeat / ping
- **Deliverable**: Device connected to HA, auth confirmed in serial log

#### B2: Entity State
- [ ] Define `AppState` singleton (lamps, ACs, room temp)
- [ ] Subscribe to HA entity state changes
- [ ] Parse incoming state → write into `AppState`
- [ ] Log state updates
- **Deliverable**: `AppState` reflects live HA entity states

#### B3: Commands
- [ ] Send lamp on/off + brightness commands
- [ ] Send AC target temp + mode commands
- [ ] Confirm HA acknowledges commands
- **Deliverable**: Device can control HA entities

#### B4: Resilience
- [ ] Reconnect WiFi on drop
- [ ] Reconnect HA WebSocket on drop
- [ ] Handle HA unavailable gracefully (show stale state)
- **Deliverable**: Device recovers from connection loss without restart

---

### Frontend Lane

#### F1: Circular Menu
- [ ] Items arranged around the ring edge (small, dimmed)
- [ ] Selected item in center (large, white)
- [ ] Dial rotation cycles selection with animation
- [ ] Button press navigates into control screen
- [ ] Back navigation (long press to return to menu)
- **Deliverable**: Polished circular navigation

#### F2: Lamp Control Screen
- [ ] Shows lamp name + current on/off state
- [ ] Button toggles on/off
- [ ] Dial adjusts brightness when on
- [ ] Reads from / writes to `AppState`
- **Deliverable**: Full lamp UI (static state for now)

#### F3: AC Control Screen
- [ ] Shows current temp + target temp + mode
- [ ] Dial adjusts target temperature
- [ ] Button cycles mode (cool → heat → auto → off)
- [ ] Reads from / writes to `AppState`
- **Deliverable**: Full AC UI (static state for now)

#### F4: Polish
- [ ] Haptic feedback on scroll and selection
- [ ] Screensaver / sleep after inactivity
- [ ] Smooth screen transition animations
- [ ] Temperature sensor display on menu or status bar
- **Deliverable**: Production-quality feel

---

### Integration

#### I1: Live Menu (B2 + F1)
- [ ] Menu items show real entity names from `AppState`
- [ ] Menu reflects current on/off state (e.g. dim if lamp is off)

#### I2: Live Lamp Control (B3 + F2)
- [ ] Lamp screen sends real commands via `HAClient`
- [ ] Screen updates when HA confirms state change

#### I3: Live AC Control (B3 + F3)
- [ ] AC screen sends real commands via `HAClient`
- [ ] Screen updates when HA confirms state change

---

## Open Questions
- Back navigation: long press center button, or dedicated gesture?
- Multiple lamps: flat list or grouped by room?
- AC modes: depends on HA climate entity config — discover at B2
- Temperature sensor: which entity, where displayed?

---
*Last updated: March 2026*
