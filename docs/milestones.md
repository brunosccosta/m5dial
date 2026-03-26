# Milestones

## Milestone 1: Hardware Validation ✅
Display works, encoder and button respond.

---

## Backend Lane

### B1: WiFi + HA Connection
- [ ] Connect to WiFi with hardcoded credentials
- [ ] Open HA WebSocket connection
- [ ] Authenticate with long-lived token
- [ ] Confirm connection with a heartbeat / ping
- **Deliverable**: Device connected to HA, auth confirmed in serial log

### B2: Entity State
- [x] Define `AppState` singleton (lamps, ACs, room temp) — structs + static dummy data done
- [ ] Subscribe to HA entity state changes
- [ ] Parse incoming state → write into `AppState`
- [ ] Log state updates
- **Deliverable**: `AppState` reflects live HA entity states

### B3: Commands
- [ ] Send lamp on/off + brightness commands
- [ ] Send AC target temp + mode commands
- [ ] Confirm HA acknowledges commands
- **Deliverable**: Device can control HA entities

### B4: Resilience
- [ ] Reconnect WiFi on drop
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

### F2: Lamp Control Screen ✅ (static)
- [x] Screen navigation stack (ScreenManager, Screen base class)
- [x] Lamp list drill-down with Go Back
- [x] Shows lamp name + on/off state + brightness
- [x] Dial adjusts brightness, button = go back
- [x] Reads from / writes to `AppState`
- [ ] Toggle on/off (pending — needs dedicated interaction)
- **Deliverable**: Full lamp UI connected to live HA (pending I2)

### F3: AC Control Screen
- [ ] Shows current temp + target temp + mode
- [ ] Dial adjusts target temperature
- [ ] Button cycles mode (cool → heat → auto → off)
- [ ] Reads from / writes to `AppState`
- **Deliverable**: Full AC UI (static state for now)

### F4: Polish
- [ ] Haptic feedback on scroll and selection
- [ ] Screensaver / sleep after inactivity
- [ ] Smooth screen transition animations
- [ ] Temperature sensor display on menu or status bar
- **Deliverable**: Production-quality feel

---

## Integration

### I1: Live Menu (B2 + F1)
- [ ] Menu items show real entity names from `AppState`
- [ ] Menu reflects current on/off state (e.g. dim if lamp is off)

### I2: Live Lamp Control (B3 + F2)
- [ ] Lamp screen sends real commands via `HAClient`
- [ ] Screen updates when HA confirms state change

### I3: Live AC Control (B3 + F3)
- [ ] AC screen sends real commands via `HAClient`
- [ ] Screen updates when HA confirms state change

---

## Open Questions
- Lamp toggle on/off: dedicated dial position (dial to 0 = off) or button double-tap?
- Multiple lamps: flat list (current) or grouped by room?
- AC modes: depends on HA climate entity config — discover at B2
- Temperature sensor: which entity, where displayed?
- Settings screen: WiFi status + IP for now; what else?

---
*Last updated: March 2026*
