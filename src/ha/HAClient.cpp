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
        if (id >= 1 && id <= _subscribeCount)
            ESP_LOGI(TAG, "subscribe_entities batch id=%d %s", id, ok ? "confirmed" : "failed");
        if (id == _kwhBaselineMsgId) {
            if (!ok) { ESP_LOGW(TAG, "kWh baseline request failed"); return; }
            JsonArray history = doc["result"]["sensor.zonneplan_usage_kwh"];
            if (history.isNull() || history.size() == 0) {
                ESP_LOGW(TAG, "kWh baseline: no data at midnight — using current as base");
                // Will be set to current reading on next parseEnergyUsageKwh
            } else {
                float base = atof(history[0]["s"] | "0");
                struct tm t; time_t now = time(nullptr); localtime_r(&now, &t);
                appState.energy.baseKwh   = base;
                appState.energy.baseDay   = t.tm_mday;
                appState.energy.baseValid = true;
                if (appState.energy.currentKwh > 0)
                    appState.energy.dailyKwh = appState.energy.currentKwh - base;
                appState.dirty = true;
                ESP_LOGI(TAG, "kWh baseline: %.3f (day %d)", base, t.tm_mday);
            }
        }
        if (id == _batteryHistoryMsgId) {
            if (!ok) { ESP_LOGW(TAG, "battery history request failed"); return; }
            JsonArray history = doc["result"]["sensor.meshcore_82b3166b70_battery_percentage_gigitower"];
            if (history.isNull() || history.size() == 0) {
                ESP_LOGW(TAG, "battery history: no data");
            } else {
                float past    = atof(history[0]["s"] | "0");
                float current = appState.meshcore.batteryPct;
                appState.meshcore.batteryDiff      = current - past;
                appState.meshcore.batteryDiffValid = true;
                appState.dirty = true;
                ESP_LOGI(TAG, "battery 24h: was=%.1f%% now=%.0f%% diff=%+.1f%%",
                         past, current, current - past);
            }
        }
    } else if (strcmp(type, "event") == 0) {
        int id = doc["id"] | 0;
        if (id < 1 || id > _subscribeCount) return;

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

// Replace multi-byte Unicode punctuation with ASCII equivalents so LVGL fonts render them.
// Handles typographic apostrophes/quotes (U+2018/2019) from macOS device names and Spotify metadata.
static void sanitizeForDisplay(char* s) {
    char* r = s;
    char* w = s;
    while (*r) {
        if ((uint8_t)r[0] == 0xE2 && (uint8_t)r[1] == 0x80 &&
            ((uint8_t)r[2] == 0x98 || (uint8_t)r[2] == 0x99)) {
            *w++ = '\''; r += 3; // U+2018/2019 → '
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

static void parseSpotify(const char* entity_id, const char* state, JsonObject attrs) {
    SpotifyState& s = appState.spotify;
    if (state) {
        strncpy(s.state, state, sizeof(s.state) - 1);
        s.state[sizeof(s.state) - 1] = '\0';
    }
    if (!attrs.isNull()) {
        const char* title = attrs["media_title"];
        if (title)  { strncpy(s.title,  title,  sizeof(s.title)  - 1); s.title[sizeof(s.title)   - 1] = '\0'; sanitizeForDisplay(s.title);  }
        const char* artist = attrs["media_artist"];
        if (artist) { strncpy(s.artist, artist, sizeof(s.artist) - 1); s.artist[sizeof(s.artist) - 1] = '\0'; sanitizeForDisplay(s.artist); }
        const char* source = attrs["source"];
        if (source) { strncpy(s.source, source, sizeof(s.source) - 1); s.source[sizeof(s.source) - 1] = '\0'; sanitizeForDisplay(s.source); }
        const char* repeat = attrs["repeat"];
        if (repeat) { strncpy(s.repeat, repeat, sizeof(s.repeat) - 1); s.repeat[sizeof(s.repeat) - 1] = '\0'; }
        if (attrs["volume_level"].is<float>()) s.volume  = attrs["volume_level"];
        if (attrs["shuffle"].is<bool>())       s.shuffle = attrs["shuffle"];
    }
    s.valid = true;
    ESP_LOGI(TAG, "spotify [%s]: %s - %s src=%s vol=%.0f%%",
             s.state, s.artist, s.title, s.source, s.volume * 100);
}

// HA last_updated has no timezone suffix — it's in device-local time.
// mktime() also interprets as local, so no compensation needed.
static time_t parseLocalTimestamp(const char* ts) {
    int y, mo, d, h, mn, s;
    if (sscanf(ts, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mn, &s) != 6) return 0;
    struct tm t = {};
    t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d;
    t.tm_hour = h; t.tm_min = mn; t.tm_sec = s; t.tm_isdst = -1;
    return mktime(&t);
}

static void parseMeshCoreBattery(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.meshcore.batteryPct = (int)atof(state);
    if (!attrs.isNull()) {
        const char* ts = attrs["last_updated"];
        if (ts) appState.meshcore.lastUpdatedAt = parseLocalTimestamp(ts);
    }
    appState.meshcore.valid = true;
    ESP_LOGI(TAG, "meshcore battery: %d%% diff=%lds",
             appState.meshcore.batteryPct, (long)(time(nullptr) - appState.meshcore.lastUpdatedAt));
}

static void parseMeshCoreUptime(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.meshcore.uptimeSeconds = (uint32_t)(atof(state) * 86400.0f);
    appState.meshcore.valid = true;
    ESP_LOGI(TAG, "meshcore uptime: %us", appState.meshcore.uptimeSeconds);
}

static void parseMeshCoreAirtime(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.meshcore.airtimeUtil = atof(state);
    appState.meshcore.valid = true;
    ESP_LOGI(TAG, "meshcore airtime: %.1f%%", appState.meshcore.airtimeUtil);
}

static void parseEnergyUsageKwh(const char* entity_id, const char* state, JsonObject attrs) {
    if (!state) return;
    float current = atof(state);
    appState.energy.currentKwh = current;
    appState.energy.valid = true;

    // Day rollover: re-request baseline when local day changes
    struct tm t; time_t now = time(nullptr); localtime_r(&now, &t);
    if (appState.energy.baseValid && t.tm_mday != appState.energy.baseDay) {
        appState.energy.baseValid = false;
        haClient.requestKwhBaseline();
    }

    if (appState.energy.baseValid) {
        appState.energy.dailyKwh = current - appState.energy.baseKwh;
    } else {
        // No baseline yet — show 0 until baseline query resolves
        appState.energy.dailyKwh = 0.0f;
    }
    ESP_LOGI(TAG, "energy kWh: meter=%.3f daily=%.3f", current, appState.energy.dailyKwh);
}

static void parseEnergyCurrentUsage(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.energy.currentW = atof(state);
    appState.energy.valid = true;
    ESP_LOGI(TAG, "energy current: %.0f W", appState.energy.currentW);
}

static void parseEnergyTariff(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.energy.tariff = atof(state);
    appState.energy.valid = true;
    ESP_LOGI(TAG, "energy tariff: %.4f", appState.energy.tariff);
}

static void parseEnergySustainScore(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.energy.sustainScore = atof(state);
    appState.energy.valid = true;
    ESP_LOGI(TAG, "energy sustainability: %.1f%%", appState.energy.sustainScore);
}

static void parseEnergyTip(const char* entity_id, const char* state, JsonObject attrs) {
    if (!state) return;
    strncpy(appState.energy.tip, state, sizeof(appState.energy.tip) - 1);
    appState.energy.tip[sizeof(appState.energy.tip) - 1] = '\0';
    appState.energy.valid = true;
    ESP_LOGI(TAG, "energy tip: %s", appState.energy.tip);
}

static void parseSolarLive(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.solar.liveW = atof(state);
    appState.solar.valid = true;
    appState.dirty = true;
    ESP_LOGI(TAG, "solar live: %.0f W", appState.solar.liveW);
}

static void parseSolarEnergyToday(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.solar.energyTodayKwh = atof(state);
    appState.solar.valid = true;
    appState.dirty = true;
    ESP_LOGI(TAG, "solar energy today: %.3f kWh", appState.solar.energyTodayKwh);
}

static void parseSolarValueToday(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.solar.valueTodayEur = atof(state);
    appState.solar.valid = true;
    appState.dirty = true;
    ESP_LOGI(TAG, "solar value today: %.4f EUR", appState.solar.valueTodayEur);
}

static void parseBatterySavingsToday(const char* entity_id, const char* state, JsonObject attrs) {
    if (state) appState.solar.savingsTodayEur = atof(state);
    appState.solar.valid = true;
    appState.dirty = true;
    ESP_LOGI(TAG, "battery savings today: %.4f EUR", appState.solar.savingsTodayEur);
}

static void parseLoveMode(const char* entity_id, const char* state, JsonObject attrs) {
    if (!state) return;
    appState.loveMode = strcmp(state, "on") == 0;
    appState.dirty    = true;
    ESP_LOGI(TAG, "loveMode: %s", appState.loveMode ? "on" : "off");
}

static const SensorEntry SENSORS[] = {
    { "input_boolean.nastya_at_home",      parseLoveMode          },
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
    { "sensor.quarto_temperature",                                 parseBedroomTemp      },
    { "sensor.quarto_humidity",                                    parseBedroomHumidity  },
    { "sensor.atc_88dc_temperature",                               parseBathroomTemp     },
    { "sensor.atc_88dc_humidity",                                  parseBathroomHumidity },
    { "sensor.meshcore_82b3166b70_battery_percentage_gigitower",   parseMeshCoreBattery  },
    { "sensor.meshcore_82b3166b70_uptime_gigitower",               parseMeshCoreUptime   },
    { "sensor.meshcore_82b3166b70_airtime_utilization_gigitower",  parseMeshCoreAirtime  },
    { "sensor.zonneplan_usage_kwh",                                parseEnergyUsageKwh   },
    { "sensor.zonneplan_current_usage",                            parseEnergyCurrentUsage },
    { "sensor.zonneplan_current_electricity_tariff",               parseEnergyTariff     },
    { "sensor.zonneplan_sustainability_score",                     parseEnergySustainScore },
    { "sensor.zonneplan_status_tip",                               parseEnergyTip        },
    { "sensor.e200_dc_input_power",                                parseSolarLive          },
    { "sensor.solar_energy_today",                                 parseSolarEnergyToday   },
    { "sensor.solar_value_today",                                  parseSolarValueToday    },
    { "sensor.battery_savings_today",                              parseBatterySavingsToday},
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

        _subscribeCount++;

        ESP_LOGD(TAG, "<< %s", buf);
        _ws.sendTXT(buf);
        ESP_LOGI(TAG, "sent subscribe_entities batch (id=%d, sensors %d-%d)", id, start, end - 1);
    }

    requestBatteryHistory();
    requestKwhBaseline();
}

void HAClient::requestKwhBaseline() {
    time_t now = time(nullptr);
    if (now < 1577836800LL) {
        ESP_LOGW(TAG, "clock not synced, skipping kWh baseline request");
        return;
    }
    // Today's local midnight → convert to UTC for HA query
    struct tm t; localtime_r(&now, &t);
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    time_t midnight = mktime(&t);
    struct tm t_utc; gmtime_r(&midnight, &t_utc);
    char startBuf[32];
    strftime(startBuf, sizeof(startBuf), "%Y-%m-%dT%H:%M:%S+00:00", &t_utc);

    _kwhBaselineMsgId = ++_msgId;
    char buf[320];
    snprintf(buf, sizeof(buf),
        "{\"id\":%d,\"type\":\"history/history_during_period\","
        "\"start_time\":\"%s\","
        "\"entity_ids\":[\"sensor.zonneplan_usage_kwh\"],"
        "\"minimal_response\":true,\"no_attributes\":true}",
        _kwhBaselineMsgId, startBuf);
    _ws.sendTXT(buf);
    ESP_LOGI(TAG, "sent kWh baseline request (id=%d, midnight=%s)", _kwhBaselineMsgId, startBuf);
}

void HAClient::requestBatteryHistory() {
    time_t now = time(nullptr);
    if (now < 1577836800LL) {
        ESP_LOGW(TAG, "clock not synced, skipping battery history request");
        return;
    }
    time_t start = now - 86400;
    struct tm t; gmtime_r(&start, &t);
    char startBuf[32];
    strftime(startBuf, sizeof(startBuf), "%Y-%m-%dT%H:%M:%S+00:00", &t);

    _batteryHistoryMsgId = ++_msgId;
    char buf[320];
    snprintf(buf, sizeof(buf),
        "{\"id\":%d,\"type\":\"history/history_during_period\","
        "\"start_time\":\"%s\","
        "\"entity_ids\":[\"sensor.meshcore_82b3166b70_battery_percentage_gigitower\"],"
        "\"minimal_response\":true,\"no_attributes\":true}",
        _batteryHistoryMsgId, startBuf);
    _ws.sendTXT(buf);
    ESP_LOGI(TAG, "sent battery history request (id=%d, start=%s)", _batteryHistoryMsgId, startBuf);
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

void HAClient::sendPlayMedia(const char* entity_id, const char* uri) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"id\":%d,\"type\":\"call_service\",\"domain\":\"media_player\","
             "\"service\":\"play_media\","
             "\"service_data\":{\"media_content_id\":\"%s\",\"media_content_type\":\"music\"},"
             "\"target\":{\"entity_id\":\"%s\"}}",
             _msgId + 1, uri, entity_id);
    if (_dryRun) {
        ESP_LOGI(TAG, "sendPlayMedia (dry run): %s", buf);
        return;
    }
    ++_msgId;
    ESP_LOGD(TAG, "<< %s", buf);
    _ws.sendTXT(buf);
}

void HAClient::sendTransferMedia(const char* target_entity_id) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"id\":%d,\"type\":\"call_service\",\"domain\":\"media_player\","
             "\"service\":\"transfer\","
             "\"target\":{\"entity_id\":\"%s\"}}",
             _msgId + 1, target_entity_id);
    if (_dryRun) {
        ESP_LOGI(TAG, "sendTransferMedia (dry run): %s", buf);
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

