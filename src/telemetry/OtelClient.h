#pragma once
#include <Arduino.h>

class OtelClient {
public:
    void begin(const char* host, uint16_t port = 4318);
    void tick();                          // call every loop()
    void recordLoopDuration(uint32_t ms); // call from main loop with each iteration's duration

private:
    void push();
    bool addGauge(void* metricsArray, const char* name, const char* unit,
                  int64_t value, uint64_t timeNs);

    const char* _host = nullptr;
    uint16_t    _port = 4318;

    uint32_t _lastPushMs = 0;
    uint32_t _maxLoopMs  = 0;

    static constexpr uint32_t PUSH_INTERVAL_MS = 60000;
};

extern OtelClient otelClient;
