#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "HAClient.h"
#include "../AppState.h"
#include "../devices.h"
#include "../Config.h"

static const char* TAG = "HA";
static constexpr uint32_t WIFI_RETRY_MS = 5000;

HAClient haClient;

void HAClient::begin(const char* ssid, const char* password,
                     const char* host, uint16_t port, const char* token) {
    _ssid     = ssid;
    _password = password;
    _host     = host;
    _port     = port;
    _token    = token;

    ESP_LOGI(TAG, "connecting to WiFi: %s", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    appState.connection = ConnectionState::WIFI_CONNECTING;
    appState.setError(ErrorKey::WIFI, WIFI_ERROR_DELAY_MS);
}

void HAClient::update() {
    switch (appState.connection) {
        case ConnectionState::WIFI_CONNECTING: handleWifiConnecting(); break;
        case ConnectionState::WIFI_CONNECTED:  handleWifiConnected();  break;
        case ConnectionState::HA_CONNECTING:   handleHaConnecting();   break;
        case ConnectionState::HA_READY:        handleHaReady();        break;
    }
}

// --- WiFi ---

void HAClient::handleWifiConnecting() {
    if (WiFi.status() == WL_CONNECTED) {
        appState.connection = ConnectionState::WIFI_CONNECTED;
        appState.clearError(ErrorKey::WIFI);
        appState.setError(ErrorKey::HA_WS, HA_WS_ERROR_DELAY_MS);
        ESP_LOGI(TAG, "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
        return;
    }
    uint32_t now = millis();
    if (now - _lastRetryMs > WIFI_RETRY_MS) {
        _lastRetryMs = now;
        ESP_LOGW(TAG, "WiFi not connected, retrying...");
        WiFi.disconnect();
        WiFi.begin(_ssid, _password);
    }
}

void HAClient::handleWifiConnected() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGW(TAG, "WiFi lost, reconnecting...");
        appState.connection = ConnectionState::WIFI_CONNECTING;
        appState.setError(ErrorKey::WIFI, WIFI_ERROR_DELAY_MS);
        _lastRetryMs = 0;
        WiFi.begin(_ssid, _password);
        return;
    }
    appState.connection = ConnectionState::HA_CONNECTING;
    connectWebSocket();
}

// --- HA WebSocket ---

void HAClient::connectWebSocket() {
    ESP_LOGI(TAG, "connecting to HA at %s:%d", _host, _port);
    _ws.begin(_host, _port, "/api/websocket");
    _ws.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
        haClient.onWsEvent(type, payload, length);
    });
    _ws.setReconnectInterval(5000);
    _ws.enableHeartbeat(15000, 3000, 2);
}

void HAClient::handleHaConnecting() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGW(TAG, "WiFi lost during HA connect");
        appState.connection = ConnectionState::WIFI_CONNECTING;
        appState.setError(ErrorKey::WIFI, WIFI_ERROR_DELAY_MS);
        _ws.disconnect();
        _lastRetryMs = 0;
        WiFi.begin(_ssid, _password);
        return;
    }
    _ws.loop();
}

void HAClient::handleHaReady() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGW(TAG, "WiFi lost while HA ready");
        appState.connection = ConnectionState::WIFI_CONNECTING;
        appState.setError(ErrorKey::WIFI, WIFI_ERROR_DELAY_MS);
        _ws.disconnect();
        _lastRetryMs = 0;
        WiFi.begin(_ssid, _password);
        return;
    }
    _ws.loop();
}

// --- WebSocket event handler ---

void HAClient::onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            ESP_LOGI(TAG, "WS connected");
            break;
        case WStype_DISCONNECTED:
            ESP_LOGW(TAG, "WS disconnected");
            if (appState.connection == ConnectionState::HA_READY ||
                appState.connection == ConnectionState::HA_CONNECTING) {
                appState.connection = ConnectionState::HA_CONNECTING;
                appState.setError(ErrorKey::HA_WS, HA_WS_ERROR_DELAY_MS);
            }
            break;
        case WStype_TEXT:
            ESP_LOGD(TAG, ">> %.*s", (int)length, (char*)payload);
            handleMessage(payload, length);
            break;
        default:
            break;
    }
}

void HAClient::handleMessage(uint8_t* payload, size_t length) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        ESP_LOGW(TAG, "JSON parse error: %s", err.c_str());
        return;
    }

    const char* type = doc["type"];
    if (!type) return;

    if (strcmp(type, "auth_required") == 0) {
        ESP_LOGI(TAG, "auth required, sending token");
        sendAuth();
    } else if (strcmp(type, "auth_ok") == 0) {
        ESP_LOGI(TAG, "auth ok — HA ready");
        appState.connection = ConnectionState::HA_READY;
        appState.clearError(ErrorKey::HA_WS);
        subscribeEntities();
    } else if (strcmp(type, "result") == 0) {
        int id  = doc["id"] | 0;
        bool ok = doc["success"] | false;
        if (id == _subscribeId)
            ESP_LOGI(TAG, "subscribe_entities %s", ok ? "confirmed" : "failed");
    } else if (strcmp(type, "event") == 0) {
        int id = doc["id"] | 0;
        if (id != _subscribeId) return;

        // Initial snapshot: event.a = added entities
        JsonObject added = doc["event"]["a"];
        if (!added.isNull()) {
            for (JsonPair kv : added) {
                updateACState(kv.key().c_str(),
                              kv.value()["s"] | (const char*)nullptr,
                              kv.value()["a"]);
            }
        }
        // Incremental diffs: event.c = changed entities, "+" = updated fields
        JsonObject changed = doc["event"]["c"];
        if (!changed.isNull()) {
            for (JsonPair kv : changed) {
                JsonObject patch = kv.value()["+"];
                if (!patch.isNull())
                    updateACState(kv.key().c_str(),
                                  patch["s"] | (const char*)nullptr,
                                  patch["a"]);
            }
        }
    } else if (strcmp(type, "auth_invalid") == 0) {
        ESP_LOGE(TAG, "auth invalid — check HA token in credentials.h");
    }
}

void HAClient::sendAuth() {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"type\":\"auth\",\"access_token\":\"[redacted]\"}");
    ESP_LOGD(TAG, "<< %s", buf);
    snprintf(buf, sizeof(buf), "{\"type\":\"auth\",\"access_token\":\"%s\"}", _token);
    _ws.sendTXT(buf);
}

void HAClient::subscribeEntities() {
    _msgId = 0;

    // Build entity_ids array from devices.h
    char entityIds[256] = "[";
    for (int i = 0; i < AC_COUNT; i++) {
        if (i > 0) strncat(entityIds, ",", sizeof(entityIds) - strlen(entityIds) - 1);
        strncat(entityIds, "\"",           sizeof(entityIds) - strlen(entityIds) - 1);
        strncat(entityIds, ACS[i].entity_id, sizeof(entityIds) - strlen(entityIds) - 1);
        strncat(entityIds, "\"",           sizeof(entityIds) - strlen(entityIds) - 1);
    }
    strncat(entityIds, "]", sizeof(entityIds) - strlen(entityIds) - 1);

    char buf[320];
    _subscribeId = ++_msgId;
    snprintf(buf, sizeof(buf),
             "{\"id\":%d,\"type\":\"subscribe_entities\",\"entity_ids\":%s}",
             _subscribeId, entityIds);
    ESP_LOGD(TAG, "<< %s", buf);
    _ws.sendTXT(buf);
    ESP_LOGI(TAG, "sent subscribe_entities (id=%d)", _subscribeId);
}

void HAClient::updateACState(const char* entity_id, const char* state, JsonObject attrs) {
    if (!entity_id) return;

    for (int i = 0; i < AC_COUNT; i++) {
        if (strcmp(appState.acs[i].entity_id, entity_id) != 0) continue;

        if (state) {
            strncpy(appState.acs[i].mode, state, sizeof(appState.acs[i].mode) - 1);
            appState.acs[i].mode[sizeof(appState.acs[i].mode) - 1] = '\0';
        }
        if (!attrs.isNull()) {
            if (attrs["current_temperature"].is<float>())
                appState.acs[i].current_temp = attrs["current_temperature"];
            if (attrs["temperature"].is<float>())
                appState.acs[i].target_temp = attrs["temperature"];
        }
        appState.acs[i].valid = true;
        appState.dirty        = true;

        ESP_LOGI(TAG, "AC[%d] %s: mode=%s cur=%.1f tgt=%.1f",
                 i, entity_id, appState.acs[i].mode,
                 appState.acs[i].current_temp, appState.acs[i].target_temp);
        return;
    }
}
