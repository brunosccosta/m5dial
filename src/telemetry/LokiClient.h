#pragma once
#include <Arduino.h>
#include <esp_log.h>

// Ships ESP_LOG output to Loki, gated by the LOKI_LOGGING compile flag.
//
// begin() installs an esp_log_set_vprintf hook that mirrors every fully-formatted
// log line into a fixed ring buffer (Serial output is preserved by forwarding to
// the previous hook). tick() batch-POSTs the pending lines to Loki's push API
// every FLUSH_INTERVAL_MS. The JSON body is built in a static buffer so log
// shipping does not itself churn the heap (see the JsonArena fragmentation fix).
//
// Compile out entirely by setting -DLOKI_LOGGING=0 in platformio.ini.
class LokiClient {
public:
    void begin(const char* host, uint16_t port); // installs the log hook
    void tick();                                  // call every loop()

    // Shipping health (pushed as OTel gauges via the independent metrics path,
    // so they stay visible even when Loki itself is unreachable).
    uint32_t linesSent()    const { return _linesSent; }
    uint32_t pushFailures() const { return _pushFailures; }
    uint32_t droppedLines() const { return _dropped; }

private:
    static int logHook(const char* fmt, va_list args);
    void capture(const char* text, size_t len);
    bool popLine(char* out, size_t cap, uint64_t& ts);
    void flush();

    static constexpr int      RING              = 24;
    static constexpr size_t   MSG_MAX          = 144;
    static constexpr uint32_t FLUSH_INTERVAL_MS = 3000;

    struct Line {
        uint64_t ts;
        uint16_t len;
        char     msg[MSG_MAX];
    };

    const char*    _host        = nullptr;
    uint16_t       _port        = 3100;
    uint32_t       _lastFlushMs = 0;
    volatile bool  _flushing    = false;

    Line          _ring[RING];
    volatile int  _head  = 0; // next write slot
    volatile int  _count = 0; // pending lines
    portMUX_TYPE  _mux   = portMUX_INITIALIZER_UNLOCKED;

    vprintf_like_t _prevHook = nullptr;

    volatile uint32_t _linesSent    = 0;
    volatile uint32_t _pushFailures = 0;
    volatile uint32_t _dropped      = 0;

    static LokiClient* _self; // for the static C-style hook
};

extern LokiClient lokiClient;
