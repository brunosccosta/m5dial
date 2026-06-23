#include "OtelClient.h"
#include "../AppState.h"
#include "../ha/HAClient.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_log.h>

static const char* TAG = "Otel";

OtelClient otelClient;

void OtelClient::begin(const char* host, uint16_t port) {
    _host = host;
    _port = port;
}

void OtelClient::tick() {
    if (!_host) return;
    if (appState.connection != ConnectionState::HA_READY &&
        appState.connection != ConnectionState::WIFI_CONNECTED) return;

    uint32_t now = millis();
    if (now - _lastPushMs < PUSH_INTERVAL_MS) return;
    _lastPushMs = now;

    push();
}

void OtelClient::recordLoopDuration(uint32_t ms) {
    if (ms > _maxLoopMs) _maxLoopMs = ms;
}

// --- OTLP/HTTP push ---------------------------------------------------------

// NOTE: no OTLP `unit` field is emitted. The Prometheus exporter appends the
// unit to the metric name (rssi_dbm → rssi_dbm_dBm), so we encode the unit in
// the name directly (Prometheus convention) and leave the OTLP unit empty.
bool OtelClient::addGauge(void* arr, const char* name,
                           int64_t value, uint64_t timeNs) {
    JsonArray& metrics = *reinterpret_cast<JsonArray*>(arr);
    JsonObject m  = metrics.add<JsonObject>();
    m["name"]     = name;
    JsonArray dp  = m["gauge"]["dataPoints"].to<JsonArray>();
    JsonObject pt = dp.add<JsonObject>();
    pt["asInt"]   = value;

    // timeUnixNano must be a decimal string (uint64 overflows JSON number)
    char tsBuf[24];
    snprintf(tsBuf, sizeof(tsBuf), "%llu", (unsigned long long)timeNs);
    pt["timeUnixNano"] = tsBuf;

    return true;
}

void OtelClient::push() {
    time_t now = time(nullptr);
    if (now < 1577836800LL) {
        ESP_LOGW(TAG, "clock not synced, skipping push");
        return;
    }
    uint64_t timeNs = (uint64_t)now * 1000000000ULL;

    // Snapshot metrics before the blocking HTTP call
    int64_t freeHeap    = (int64_t)ESP.getFreeHeap();
    int64_t minFreeHeap = (int64_t)ESP.getMinFreeHeap();
    int64_t largestBlk  = (int64_t)ESP.getMaxAllocHeap();
    int64_t maxLoop     = (int64_t)_maxLoopMs;
    int64_t rssi        = (int64_t)WiFi.RSSI();
    int64_t haConn      = (appState.connection == ConnectionState::HA_READY) ? 1 : 0;
    int64_t reconnects  = (int64_t)haClient.wsReconnects();
    int64_t uptime      = (int64_t)(millis() / 1000);
    _maxLoopMs = 0; // reset after snapshot

    JsonDocument doc;
    JsonArray resourceMetrics = doc["resourceMetrics"].to<JsonArray>();
    JsonObject rm = resourceMetrics.add<JsonObject>();

    // Resource attributes
    JsonArray resAttrs = rm["resource"]["attributes"].to<JsonArray>();
    JsonObject svcName = resAttrs.add<JsonObject>();
    svcName["key"]                      = "service.name";
    svcName["value"]["stringValue"]     = "m5dial";
    JsonObject devId = resAttrs.add<JsonObject>();
    devId["key"]                        = "service.instance.id";
    devId["value"]["stringValue"]       = "m5dial-01";

    // Scope + metrics
    JsonArray scopeMetrics = rm["scopeMetrics"].to<JsonArray>();
    JsonObject sm = scopeMetrics.add<JsonObject>();
    sm["scope"]["name"] = "m5dial/firmware";
    JsonArray metrics   = sm["metrics"].to<JsonArray>();

    addGauge(&metrics, "m5dial_memory_free_bytes",              freeHeap,    timeNs);
    addGauge(&metrics, "m5dial_memory_min_free_bytes",          minFreeHeap, timeNs);
    addGauge(&metrics, "m5dial_memory_largest_block_bytes",     largestBlk,  timeNs);
    addGauge(&metrics, "m5dial_loop_max_duration_milliseconds", maxLoop,     timeNs);
    addGauge(&metrics, "m5dial_wifi_rssi_dbm",                  rssi,        timeNs);
    addGauge(&metrics, "m5dial_ha_connected",                   haConn,      timeNs);
    addGauge(&metrics, "m5dial_ha_ws_reconnects_total",         reconnects,  timeNs);
    addGauge(&metrics, "m5dial_uptime_seconds_total",           uptime,      timeNs);

    String body;
    serializeJson(doc, body);

    WiFiClient   wifiClient;
    HTTPClient   http;
    http.setConnectTimeout(5000);
    http.setTimeout(8000);

    char url[64];
    snprintf(url, sizeof(url), "http://%s:%d/v1/metrics", _host, _port);
    http.begin(wifiClient, url);
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(body);
    if (code == 200 || code == 204) {
        ESP_LOGI(TAG, "pushed metrics ok (%d), heap=%lld maxLoop=%lldms",
                 code, freeHeap, maxLoop);
    } else {
        ESP_LOGW(TAG, "push failed: HTTP %d", code);
    }
    http.end();
}
