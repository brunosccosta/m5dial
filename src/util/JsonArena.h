#pragma once
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>

// Fixed static-arena allocator for ArduinoJson v7 (no PSRAM on M5Dial).
//
// Why: every WS message / OTel push / Spotify response creates a JsonDocument
// whose pool is heap-allocated and freed on scope exit. Those variable-size
// alloc/free cycles punch permanent holes in the single ~150KB heap arena —
// the largest contiguous block ratchets down until a big alloc fails and the
// firmware hard-hangs (see docs/learnings.md "Heap fragmentation causes hard
// freezes"). A monotonic bump arena reused for every parse removes that churn.
//
// Lifecycle assumption: all JsonDocuments using this arena are short-lived and
// live on the loop() thread, never nested (one parse completes and is destroyed
// before the next begins). When the live-block count returns to zero the arena
// resets to its base, so the same bytes are reused every message — zero heap
// churn in the common case.
//
// Safety: if a message is larger than the arena it transparently falls back to
// malloc/realloc/free, so correctness is preserved (we never corrupt or crash);
// only that one oversized message pays the heap cost. Each arena block carries
// an 8-byte size header so reallocate() can copy correctly.
class JsonArena : public ArduinoJson::Allocator {
public:
    void* allocate(size_t size) override {
        size_t need = HDR + alignUp(size);
        if (_off + need <= CAP) {
            uint8_t* base = _buf + _off;
            *reinterpret_cast<size_t*>(base) = size;
            _off += need;
            if (_off > _highWater) _highWater = _off;
            _live++;
            return base + HDR;
        }
        _overflows++;
        return malloc(size); // rare: oversized message, heap fallback
    }

    // Telemetry: peak bytes ever used (vs CAP) and count of heap fallbacks.
    // Pushed as OTel gauges so the dashboard can confirm CAP is sized right
    // and that the arena is actually absorbing the churn (overflows ~ 0).
    size_t   capacity()  const { return CAP; }
    size_t   highWater() const { return _highWater; }
    uint32_t overflows() const { return _overflows; }

    void deallocate(void* ptr) override {
        if (!ptr) return;
        if (inArena(ptr)) {
            if (--_live == 0) _off = 0; // all freed → reuse arena from base
        } else {
            free(ptr);
        }
    }

    void* reallocate(void* ptr, size_t newSize) override {
        if (!ptr) return allocate(newSize);
        if (!inArena(ptr)) return realloc(ptr, newSize);

        uint8_t* base = static_cast<uint8_t*>(ptr) - HDR;
        size_t   old  = *reinterpret_cast<size_t*>(base);

        // Grow/shrink in place if this is the most recent arena block.
        if (base + HDR + alignUp(old) == _buf + _off) {
            size_t need = HDR + alignUp(newSize);
            if (base + need <= _buf + CAP) {
                *reinterpret_cast<size_t*>(base) = newSize;
                _off = static_cast<size_t>((base + need) - _buf);
                return ptr;
            }
        }

        // Otherwise move: allocate fresh, copy, abandon the old block.
        void* np = allocate(newSize);
        if (!np) return nullptr;
        memcpy(np, ptr, old < newSize ? old : newSize);
        _live--; // old block is abandoned and will never be deallocated
        return np;
    }

private:
    // 24KB. 12KB pegged high-water at the ceiling with overflow_total > 0 within
    // a minute of boot; source not yet attributed (shared across HAClient,
    // OtelClient, SpotifyClient — suspected SpotifyClient, which stream-parses
    // and copies strings into the pool, but unconfirmed). A larger static .bss
    // reservation does NOT fragment the heap the way per-message churn did —
    // watch m5dial_json_arena_high_water_bytes / _overflow_total to tune.
    static constexpr size_t CAP   = 24576;
    static constexpr size_t ALIGN = 8;
    static constexpr size_t HDR   = ALIGN; // per-block size header (kept aligned)

    static size_t alignUp(size_t n) { return (n + (ALIGN - 1)) & ~(ALIGN - 1); }
    bool inArena(void* p) const { return p >= _buf && p < _buf + CAP; }

    uint8_t  _buf[CAP];
    size_t   _off       = 0;
    int      _live      = 0;
    size_t   _highWater = 0;
    uint32_t _overflows = 0;
};

// Single shared arena for all loop()-thread JsonDocuments (non-nested usage).
inline JsonArena& sharedJsonArena() {
    static JsonArena arena;
    return arena;
}
