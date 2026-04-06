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
                     const char* host, uint16_t port, const char* token,
                     bool dryRun) {
    _ssid     = ssid;
    _password = password;
    _host     = host;
    _port     = port;
    _token    = token;
    _dryRun   = dryRun;

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
        int  id = doc["id"] | 0;
        bool ok = doc["success"] | false;
        for (int i = 0; i < _subscribeCount; i++) {
            if (_subscribeIds[i] == id)
                ESP_LOGI(TAG, "subscribe_entities batch id=%d %s", id, ok ? "confirmed" : "failed");
        }
    } else if (strcmp(type, "event") == 0) {
        int id = doc["id"] | 0;
        bool known = false;
        for (int i = 0; i < _subscribeCount; i++)
            if (_subscribeIds[i] == id) { known = true; break; }
        if (!known) return;

        // Initial snapshot: event.a = added entities
        JsonObject added = doc["event"]["a"];
        if (!added.isNull()) {
            for (JsonPair kv : added)
                dispatchSensor(kv.key().c_str(), kv.value()["s"] | (const char*)nullptr, kv.value()["a"]);
        }
        // Incremental diffs: event.c = changed entities, "+" = updated fields
        JsonObject changed = doc["event"]["c"];
        if (!changed.isNull()) {
            for (JsonPair kv : changed) {
                JsonObject patch = kv.value()["+"];
                if (!patch.isNull())
                    dispatchSensor(kv.key().c_str(), patch["s"] | (const char*)nullptr, patch["a"]);
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

// --- Sensor registry ---

struct SensorEntry {
    const char* entity_id;
    void (*parse)(const char* entity_id, const char* state, JsonObject attrs);
};

static void parseAC(const char* entity_id, const char* state, JsonObject attrs) {
    bool isMain = strcmp(entity_id, "climate.forninho_room_temperature") == 0;
    ACState& ac = isMain ? appState.ac : appState.heater;
    ac.entity_id = isMain ? "climate.forninho_room_temperature" : "climate.forninho_portatil";
    if (state) {
        strncpy(ac.mode, state, sizeof(ac.mode) - 1);
        ac.mode[sizeof(ac.mode) - 1] = '\0';
    }
    if (!attrs.isNull()) {
        if (attrs["current_temperature"].is<float>()) ac.current_temp = attrs["current_temperature"];
        if (attrs["temperature"].is<float>())         ac.target_temp  = attrs["temperature"];
        JsonArray modes = attrs["hvac_modes"];
        if (!modes.isNull() && ac.modeCount == 0) {
            int count = 0;
            for (JsonVariant m : modes) {
                if (count >= 6) break;
                strncpy(ac.availableModes[count], m.as<const char*>(), 11);
                ac.availableModes[count][11] = '\0';
                count++;
            }
            ac.modeCount = count;
        }
    }
    ac.valid = true;
    ESP_LOGI(TAG, "AC [%s]: mode=%s cur=%.1f tgt=%.1f", entity_id, ac.mode, ac.current_temp, ac.target_temp);
}

static void parseWeather(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) {
        strncpy(appState.weather.condition, state, sizeof(appState.weather.condition) - 1);
        appState.weather.condition[sizeof(appState.weather.condition) - 1] = '\0';
    }
    if (!attrs.isNull()) {
        if (attrs["temperature"].is<float>())          appState.weather.temperature = attrs["temperature"];
        if (attrs["apparent_temperature"].is<float>()) appState.weather.feelsLike   = attrs["apparent_temperature"];
    }
    appState.weather.valid = true;
    ESP_LOGI(TAG, "weather: %s %.1f° feels %.1f°", appState.weather.condition, appState.weather.temperature, appState.weather.feelsLike);
}

static void parseDetailedCondition(const char* entity_id, const char* state, JsonObject attrs) {
    if (!state) return;
    strncpy(appState.weather.detailedCondition, state, sizeof(appState.weather.detailedCondition) - 1);
    appState.weather.detailedCondition[sizeof(appState.weather.detailedCondition) - 1] = '\0';
    ESP_LOGI(TAG, "detailed condition: %s", appState.weather.detailedCondition);
}

static void parseOutdoorTemp(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.sensors.outdoorTemp = atof(state);
    ESP_LOGI(TAG, "outdoor temp: %.1f°C", appState.sensors.outdoorTemp);
}

static void parseOutdoorHumidity(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.sensors.outdoorHumidity = atof(state);
    ESP_LOGI(TAG, "outdoor humidity: %.0f%%", appState.sensors.outdoorHumidity);
}

static void parseBedroomTemp(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.sensors.bedroomTemp = atof(state);
    ESP_LOGI(TAG, "bedroom temp: %.1f°C", appState.sensors.bedroomTemp);
}

static void parseBedroomHumidity(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.sensors.bedroomHumidity = atof(state);
    ESP_LOGI(TAG, "bedroom humidity: %.0f%%", appState.sensors.bedroomHumidity);
}

static void parseBathroomTemp(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.sensors.bathroomTemp = atof(state);
    ESP_LOGI(TAG, "bathroom temp: %.1f°C", appState.sensors.bathroomTemp);
}

static void parseBathroomHumidity(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.sensors.bathroomHumidity = atof(state);
    ESP_LOGI(TAG, "bathroom humidity: %.0f%%", appState.sensors.bathroomHumidity);
}

static void parseSun(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.weather.isDaytime = (strcmp(state, "above_horizon") == 0);
    ESP_LOGI(TAG, "sun: %s", appState.weather.isDaytime ? "day" : "night");
}

static void parseForecastCondition(const char* entity_id, const char* state, JsonObject attrs) {
    if (!state) return;
    ForecastDay& f = strstr(entity_id, "_2d") ? appState.forecastTomorrow : appState.forecastToday;
    strncpy(f.detailedCondition, state, sizeof(f.detailedCondition) - 1);
    f.detailedCondition[sizeof(f.detailedCondition) - 1] = '\0';
    f.valid = true;
    ESP_LOGI(TAG, "forecast condition [%s]: %s", entity_id, f.detailedCondition);
}

static void parseForecastTemp(const char* entity_id, const char* state, JsonObject attrs) {
    if (!state) return;
    ForecastDay& f = strstr(entity_id, "_2d") ? appState.forecastTomorrow : appState.forecastToday;
    f.temperature = atof(state);
    ESP_LOGI(TAG, "forecast temp [%s]: %.1f", entity_id, f.temperature);
}

static void parseForecastRain(const char* entity_id, const char* state, JsonObject attrs) {
    if (!state) return;
    ForecastDay& f = strstr(entity_id, "_2d") ? appState.forecastTomorrow : appState.forecastToday;
    f.rainChance = atoi(state);
    ESP_LOGI(TAG, "forecast rain [%s]: %d%%", entity_id, f.rainChance);
}

static void parseSpotify(const char* entity_id, const char* state, JsonObject attrs) {
    SpotifyState& s = appState.spotify;
    if (state) {
        strncpy(s.state, state, sizeof(s.state) - 1);
        s.state[sizeof(s.state) - 1] = '\0';
    }
    if (!attrs.isNull()) {
        const char* title = attrs["media_title"];
        if (title)  { strncpy(s.title,  title,  sizeof(s.title)  - 1); s.title[sizeof(s.title)   - 1] = '\0'; }
        const char* artist = attrs["media_artist"];
        if (artist) { strncpy(s.artist, artist, sizeof(s.artist) - 1); s.artist[sizeof(s.artist) - 1] = '\0'; }
        const char* source = attrs["source"];
        if (source) { strncpy(s.source, source, sizeof(s.source) - 1); s.source[sizeof(s.source) - 1] = '\0'; }
        const char* repeat = attrs["repeat"];
        if (repeat) { strncpy(s.repeat, repeat, sizeof(s.repeat) - 1); s.repeat[sizeof(s.repeat) - 1] = '\0'; }
        if (attrs["volume_level"].is<float>()) s.volume  = attrs["volume_level"];
        if (attrs["shuffle"].is<bool>())       s.shuffle = attrs["shuffle"];
    }
    s.valid = true;
    ESP_LOGI(TAG, "spotify [%s]: %s - %s src=%s vol=%.0f%%",
             s.state, s.artist, s.title, s.source, s.volume * 100);
}

static const SensorEntry SENSORS[] = {
    { "media_player.spotify",              parseSpotify           },
    { "climate.forninho_room_temperature", parseAC                },
    { "climate.forninho_portatil",         parseAC                },
    { "weather.buienradar",                parseWeather           },
    { "sensor.detailed_condition",         parseDetailedCondition },
    { "sensor.atc_3294_temperature",       parseOutdoorTemp       },
    { "sensor.atc_3294_humidity",          parseOutdoorHumidity   },
    { "sun.sun",                           parseSun               },
    { "sensor.detailed_condition_1d",      parseForecastCondition },
    { "sensor.detailed_condition_2d",      parseForecastCondition },
    { "sensor.temperature_1d",             parseForecastTemp      },
    { "sensor.temperature_2d",             parseForecastTemp      },
    { "sensor.rainchance_1d",              parseForecastRain      },
    { "sensor.rainchance_2d",              parseForecastRain      },
    { "sensor.quarto_temperature",         parseBedroomTemp       },
    { "sensor.quarto_humidity",            parseBedroomHumidity   },
    { "sensor.atc_88dc_temperature",       parseBathroomTemp      },
    { "sensor.atc_88dc_humidity",          parseBathroomHumidity  },
};
static constexpr int SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);

void HAClient::subscribeEntities() {
    _msgId           = 0;
    _subscribeCount  = 0;

    for (int start = 0; start < SENSOR_COUNT; start += BATCH_SIZE) {
        int end = start + BATCH_SIZE;
        if (end > SENSOR_COUNT) end = SENSOR_COUNT;

        char buf[512];
        int  id  = ++_msgId;
        int  pos = snprintf(buf, sizeof(buf), "{\"id\":%d,\"type\":\"subscribe_entities\",\"entity_ids\":[", id);

        for (int i = start; i < end; i++) {
            if (i > start) buf[pos++] = ',';
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\"%s\"", SENSORS[i].entity_id);
        }
        pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");

        if (_subscribeCount < MAX_BATCHES)
            _subscribeIds[_subscribeCount++] = id;

        ESP_LOGD(TAG, "<< %s", buf);
        _ws.sendTXT(buf);
        ESP_LOGI(TAG, "sent subscribe_entities batch (id=%d, sensors %d-%d)", id, start, end - 1);
    }
}

void HAClient::dispatchSensor(const char* entity_id, const char* state, JsonObject attrs) {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (strcmp(SENSORS[i].entity_id, entity_id) == 0) {
            SENSORS[i].parse(entity_id, state, attrs);
            appState.dirty = true;
            return;
        }
    }
    ESP_LOGI(TAG, "unregistered sensor [%s] state=%s", entity_id, state ? state : "(null)");
}

void HAClient::sendACTemperature(const char* entity_id, float temp) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"id\":%d,\"type\":\"call_service\",\"domain\":\"climate\","
             "\"service\":\"set_temperature\","
             "\"service_data\":{\"temperature\":%.1f},"
             "\"target\":{\"entity_id\":\"%s\"}}",
             _msgId + 1, temp, entity_id);
    if (_dryRun) {
        ESP_LOGI(TAG, "sendACTemperature (dry run): %s", buf);
        return;
    }
    ++_msgId;
    ESP_LOGD(TAG, "<< %s", buf);
    _ws.sendTXT(buf);
}

void HAClient::sendFindMyIPhone(const char* account, const char* deviceName) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"id\":%d,\"type\":\"call_service\",\"domain\":\"icloud\","
             "\"service\":\"play_sound\","
             "\"service_data\":{\"account\":\"%s\",\"device_name\":\"%s\"}}",
             _msgId + 1, account, deviceName);
    if (_dryRun) {
        ESP_LOGI(TAG, "sendFindMyIPhone (dry run): %s", buf);
        return;
    }
    ++_msgId;
    ESP_LOGD(TAG, "<< %s", buf);
    _ws.sendTXT(buf);
}

void HAClient::sendACMode(const char* entity_id, const char* mode) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"id\":%d,\"type\":\"call_service\",\"domain\":\"climate\","
             "\"service\":\"set_hvac_mode\","
             "\"service_data\":{\"hvac_mode\":\"%s\"},"
             "\"target\":{\"entity_id\":\"%s\"}}",
             _msgId + 1, mode, entity_id);
    if (_dryRun) {
        ESP_LOGI(TAG, "sendACMode (dry run): %s", buf);
        return;
    }
    ++_msgId;
    ESP_LOGD(TAG, "<< %s", buf);
    _ws.sendTXT(buf);
}

