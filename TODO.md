# TODO

Small improvements and known issues parked for later.

---

## HAClient — subscribeEntities batching

**File:** `src/ha/HAClient.cpp` — `subscribeEntities()`

**Problem:** Currently builds a single `subscribe_entities` message with all entity IDs.
Two buffer limits that will bite as we add more sensors:
- `entityIds[512]` — the entity IDs array string
- `buf[320]` — the full WS message (must fit entityIds + wrapper — already undersized, hasn't blown up yet because current total is ~246 chars)

**Fix:** Send entities in batches of N (configurable param, default ~5).
- `_subscribeId` → `_subscribeIds[]` array tracking all active subscription IDs
- Event handler checks `id` against the array instead of single equality check
- Wrapper stays the same; only the send loop and ID tracking change

---
