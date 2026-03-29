# Milestones

## Milestone 1: Hardware Validation ✅
Display works, encoder and button respond.

---

## Backend Lane

### B1: WiFi + HA Connection ✅
- [x] Connect to WiFi with hardcoded credentials — non-blocking state machine, logs IP
- [x] Open HA WebSocket connection (`ws://host:8123/api/websocket`)
- [x] Authenticate with long-lived token — handles auth_required/auth_ok/auth_invalid
- [x] Heartbeat via WS ping frames (15s interval, auto-reconnect on drop)
- **Deliverable**: Device connected to HA, auth confirmed in serial log

### B2: Entity State ✅ (AC only — lamps deferred)
- [x] Define `AppState` singleton (ACs, room temp) — structs populated from `devices.h`
- [x] Subscribe to HA entity state via `subscribe_entities` (targeted, not full `get_states`)
- [x] Parse incoming state → write into `AppState` (initial snapshot + incremental diffs)
- [x] Log state updates
- [ ] Lamp state — deferred until lamp entities are added to `devices.h`
- **Deliverable**: `AppState` reflects live AC states from HA

### B3: Commands
- [ ] ~~Send lamp on/off + brightness commands~~ — deferred with lamps
- [ ] Send AC target temp + mode commands
- [ ] Confirm HA acknowledges commands
- **Deliverable**: Device can control AC entities

### B4: Resilience
- [x] `ConnectionState` enum in `AppState` — written by HAClient, readable by UI
- [x] WiFi retry on drop (auto-reconnect in HAClient state machine)
- [ ] Reconnect HA WebSocket on drop
- [ ] Handle HA unavailable gracefully (show stale state)
- **Deliverable**: Device recovers from connection loss without restart

---

## Frontend Lane

### F1: Circular Menu ✅
- [x] Items arranged around the ring edge (small, dimmed)
- [x] Selected item in center (large icon + label)
- [x] Dial rotation cycles selection with animation
- [x] Button press navigates into control screen
- [x] Back navigation via "Go Back" ring item (replaces long press)

### F2: Lamp Control Screen — deferred
All lamp UI work is deferred until lamps are added to `devices.h` and B2 lamp state is live.

### F3: AC Control Screen
- [ ] Shows current temp + target temp + mode
- [ ] Dial adjusts target temperature
- [ ] Button cycles mode (cool → heat → auto → off)
- [ ] Reads from / writes to `AppState`
- **Deliverable**: Full AC UI connected to live HA

### F4: Error Overlay ✅
- [x] Generic LVGL overlay shown on top of any active screen (`lv_layer_top()`)
- [x] Triggered by error registry in `AppState` — any subsystem can push/clear errors by key
- [x] Disappears automatically when all errors clear
- [x] Shows appropriate icon + short message per error type
- [x] Collapses to a small dot after 4s — stays visible without blocking navigation
- [x] Per-error `fireAfterMs` grace period — no flash at boot during normal connect
- **Deliverable**: Device surfaces connection errors without disrupting navigation

### F5: Polish
- [ ] Haptic feedback on scroll and selection
- [ ] Screensaver / sleep after inactivity
- [ ] Smooth screen transition animations
- [ ] Temperature sensor display on menu or status bar
- **Deliverable**: Production-quality feel

---

## Integration

### I1: Live Menu — deferred with lamps
Depends on lamp entities being live in `AppState`.

### I2: Live Lamp Control — deferred with lamps
Depends on F2 and B3 lamp commands.

### I3: Live AC Control (B3 + F3)
- [ ] AC screen sends real commands via `HAClient`
- [ ] Screen updates when HA confirms state change

---

## Open Questions
- Lamps: deferred — add entity IDs to `devices.h` when ready to pick this up
- AC modes: supported modes vary by device — read `hvac_modes` attribute if needed
- Temperature sensor: which entity, where displayed?
- Settings screen: WiFi status + IP for now; what else?

---
*Last updated: March 2026*
