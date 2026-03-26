#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "HAClient.h"
#include "../AppState.h"
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
    } else if (strcmp(type, "auth_invalid") == 0) {
        ESP_LOGE(TAG, "auth invalid — check HA token in credentials.h");
    }
}

void HAClient::sendAuth() {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"type\":\"auth\",\"access_token\":\"%s\"}", _token);
    _ws.sendTXT(buf);
}
