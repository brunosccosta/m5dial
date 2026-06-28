#include "LokiClient.h"
#if LOKI_LOGGING
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_timer.h>
#include "../util/JsonArena.h"

LokiClient lokiClient;
LokiClient* LokiClient::_self = nullptr;

void LokiClient::begin(const char* host, uint16_t port) {
    _host = host;
    _port = port;
    _self = this;
    // Install our hook; keep the previous one so Serial output is unaffected.
    _prevHook = esp_log_set_vprintf(&LokiClient::logHook);
}

// Runs in whatever task emitted the log (loop, WiFi, etc.) — keep it cheap and
// never log from here (infinite recursion). Forwards to the original sink first.
int LokiClient::logHook(const char* fmt, va_list args) {
    va_list cp;
    va_copy(cp, args);
    int r = _self && _self->_prevHook ? _self->_prevHook(fmt, args)
                                      : vprintf(fmt, args);
    if (_self && !_self->_flushing) {
        char buf[MSG_MAX];
        int n = vsnprintf(buf, sizeof(buf), fmt, cp);
        if (n > 0) _self->capture(buf, (size_t)(n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1));
    }
    va_end(cp);
    return r;
}

void LokiClient::capture(const char* text, size_t len) {
    while (len && (text[len - 1] == '\n' || text[len - 1] == '\r')) len--;
    if (!len) return;
    if (len >= MSG_MAX) len = MSG_MAX - 1;

    // Loki timestamps are unix nanoseconds; blend seconds with a sub-second
    // monotonic part so lines within the same second stay ordered.
    uint64_t ts = (uint64_t)time(nullptr) * 1000000000ULL +
                  (uint64_t)(esp_timer_get_time() % 1000000LL) * 1000ULL;

    portENTER_CRITICAL(&_mux);
    int idx = _head;
    _ring[idx].ts  = ts;
    _ring[idx].len = (uint16_t)len;
    memcpy(_ring[idx].msg, text, len);
    _ring[idx].msg[len] = '\0';
    _head = (_head + 1) % RING;
    if (_count < RING) _count++;
    else _dropped++; // ring full → oldest line overwritten
    portEXIT_CRITICAL(&_mux);
}

bool LokiClient::popLine(char* out, size_t cap, uint64_t& ts) {
    bool got = false;
    portENTER_CRITICAL(&_mux);
    if (_count > 0) {
        int tail = (_head - _count + RING) % RING;
        ts = _ring[tail].ts;
        size_t n = _ring[tail].len;
        if (n >= cap) n = cap - 1;
        memcpy(out, _ring[tail].msg, n);
        out[n] = '\0';
        _count--;
        got = true;
    }
    portEXIT_CRITICAL(&_mux);
    return got;
}

void LokiClient::tick() {
    if (!_host) return;
    uint32_t now = millis();
    if (now - _lastFlushMs < FLUSH_INTERVAL_MS) return;
    _lastFlushMs = now;
    flush();
}

void LokiClient::flush() {
    if (_count == 0) return;
    if (WiFi.status() != WL_CONNECTED) return; // hold lines until reconnected
    if (time(nullptr) < 1577836800LL) return;  // clock not synced → bad timestamps

    _flushing = true; // suppress capturing our own (and HTTPClient's) log lines

    JsonDocument doc(&sharedJsonArena());
    JsonObject stream = doc["streams"].add<JsonObject>();
    JsonObject labels = stream["stream"].to<JsonObject>();
    labels["service_name"] = "m5dial";
    labels["instance"]     = "m5dial-01";
    JsonArray values = stream["values"].to<JsonArray>();

    char     line[MSG_MAX];
    char     tsBuf[24];
    uint64_t ts;
    int      sent = 0;
    while (popLine(line, sizeof(line), ts)) {
        snprintf(tsBuf, sizeof(tsBuf), "%llu", (unsigned long long)ts);
        JsonArray entry = values.add<JsonArray>();
        entry.add(tsBuf); // char[] → ArduinoJson copies into the arena pool
        entry.add(line);
        sent++;
    }

    static char body[6144]; // static: no heap churn building the payload
    size_t bodyLen = serializeJson(doc, body, sizeof(body));

    WiFiClient wifiClient;
    HTTPClient http;
    http.setConnectTimeout(5000);
    http.setTimeout(8000);

    char url[80];
    snprintf(url, sizeof(url), "http://%s:%d/loki/api/v1/push", _host, _port);
    http.begin(wifiClient, url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST((uint8_t*)body, bodyLen);
    http.end();

    // Loki returns 204 on success. Logged at _flushing=true so it reaches Serial
    // but is not itself captured/shipped (no feedback loop).
    if (code == 204 || code == 200) {
        _linesSent += sent;
        ESP_LOGI("Loki", "pushed %d lines (%d), %u bytes", sent, code, (unsigned)bodyLen);
    } else {
        _pushFailures++;
        _dropped += sent; // popped from the ring but never delivered
        ESP_LOGW("Loki", "push failed: HTTP %d (%d lines dropped)", code, sent);
    }

    _flushing = false;
}
#endif // LOKI_LOGGING
