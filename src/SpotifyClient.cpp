#include "SpotifyClient.h"
#include "credentials.h"
#include "AppState.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <mbedtls/base64.h>
#include <esp_log.h>
#include <string.h>
#include "util/JsonArena.h"

static const char* TAG       = "Spotify";
static const char* NVS_NS    = "spotify";
static const char* NVS_KEY   = "refresh_token";

SpotifyClient spotifyClient;

void SpotifyClient::begin() {
    Preferences prefs;
    prefs.begin(NVS_NS, false); // false = create namespace if not exists; getString returns default if key absent
    String rt = prefs.getString(NVS_KEY, "");
    prefs.end();

    if (rt.length() > 0) {
        strncpy(_refreshToken, rt.c_str(), sizeof(_refreshToken) - 1);
        ESP_LOGI(TAG, "refresh token loaded from NVS");
    } else {
        strncpy(_refreshToken, SPOTIFY_REFRESH_TOKEN, sizeof(_refreshToken) - 1);
        ESP_LOGI(TAG, "refresh token loaded from credentials.h");
    }
}

void SpotifyClient::update() {
    if (WiFi.status() != WL_CONNECTED) return;
    const char* s = appState.spotify.state;
    if (strcmp(s, "playing") != 0 && strcmp(s, "paused") != 0) return;
    if (_tokenValid && (int32_t)(millis() - _tokenExpiresAt) < 0) return;
    refreshAccessToken();
}

void SpotifyClient::beginHttp(HTTPClient& http, WiFiClientSecure& client, const char* url) {
    client.setInsecure();
    http.setConnectTimeout(5000);
    http.setTimeout(8000);
    http.begin(client, url);
}

bool SpotifyClient::refreshAccessToken() {
    // Build Authorization: Basic base64(client_id:client_secret)
    char src[128];
    snprintf(src, sizeof(src), "%s:%s", SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET);
    uint8_t b64[192]; size_t b64len;
    mbedtls_base64_encode(b64, sizeof(b64), &b64len, (uint8_t*)src, strlen(src));
    b64[b64len] = '\0';

    char authHeader[210];
    snprintf(authHeader, sizeof(authHeader), "Basic %s", (char*)b64);

    char body[300];
    snprintf(body, sizeof(body), "grant_type=refresh_token&refresh_token=%s", _refreshToken);

    WiFiClientSecure client;
    HTTPClient http;
    beginHttp(http, client, "https://accounts.spotify.com/api/token");
    http.addHeader("Authorization", authHeader);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int code = http.POST(body);
    if (code != 200) {
        ESP_LOGW(TAG, "token refresh failed: HTTP %d", code);
        http.end();
        _tokenExpiresAt = millis() + 30000; // retry in 30s
        return false;
    }

    JsonDocument doc(&sharedJsonArena());
    deserializeJson(doc, http.getStream());
    http.end();

    const char* at = doc["access_token"];
    if (!at) {
        ESP_LOGW(TAG, "no access_token in response");
        _tokenExpiresAt = millis() + 30000;
        return false;
    }
    strncpy(_accessToken, at, sizeof(_accessToken) - 1);

    int expiresIn   = doc["expires_in"] | 3600;
    _tokenExpiresAt = millis() + (uint32_t)(expiresIn - 60) * 1000;
    _tokenValid     = true;
    ESP_LOGI(TAG, "token refreshed, expires in %ds", expiresIn);

    // Persist rotated refresh token if Spotify issued a new one
    const char* newRt = doc["refresh_token"];
    if (newRt && strcmp(newRt, _refreshToken) != 0) {
        strncpy(_refreshToken, newRt, sizeof(_refreshToken) - 1);
        Preferences prefs;
        prefs.begin(NVS_NS, false);
        prefs.putString(NVS_KEY, _refreshToken);
        prefs.end();
        ESP_LOGI(TAG, "refresh token rotated, saved to NVS");
    }

    return true;
}

bool SpotifyClient::ensureToken() {
    if (_tokenValid && (int32_t)(millis() - _tokenExpiresAt) < 0) return true;
    return refreshAccessToken();
}

bool SpotifyClient::put(const char* path, const char* body) {
    if (!ensureToken()) {
        ESP_LOGW(TAG, "PUT %s skipped — no valid token", path);
        return false;
    }

    char url[128];
    snprintf(url, sizeof(url), "https://api.spotify.com%s", path);

    char authHeader[320];
    snprintf(authHeader, sizeof(authHeader), "Bearer %s", _accessToken);

    WiFiClientSecure client;
    HTTPClient http;
    beginHttp(http, client, url);
    http.addHeader("Authorization", authHeader);
    http.addHeader("Content-Type", "application/json");

    int code = http.sendRequest("PUT", (uint8_t*)body, strlen(body));
    ESP_LOGI(TAG, "PUT %s → %d", path, code);
    http.end();

    return code == 200 || code == 204;
}

bool SpotifyClient::play(const char* uri) {
    char body[200];
    // Tracks and episodes require "uris" array; everything else uses "context_uri"
    if (strncmp(uri, "spotify:track:", 14) == 0 || strncmp(uri, "spotify:episode:", 16) == 0) {
        snprintf(body, sizeof(body), "{\"uris\":[\"%s\"]}", uri);
    } else {
        snprintf(body, sizeof(body), "{\"context_uri\":\"%s\"}", uri);
    }
    return put("/v1/me/player/play", body);
}

bool SpotifyClient::transfer(const char* device_id) {
    char body[128];
    snprintf(body, sizeof(body), "{\"device_ids\":[\"%s\"],\"play\":true}", device_id);
    return put("/v1/me/player", body);
}

void SpotifyClient::logDevices() {
    if (!ensureToken()) return;

    char authHeader[320];
    snprintf(authHeader, sizeof(authHeader), "Bearer %s", _accessToken);

    WiFiClientSecure client;
    HTTPClient http;
    beginHttp(http, client, "https://api.spotify.com/v1/me/player/devices");
    http.addHeader("Authorization", authHeader);

    int code = http.GET();
    if (code != 200) {
        ESP_LOGW(TAG, "logDevices failed: HTTP %d", code);
        http.end();
        return;
    }

    JsonDocument doc(&sharedJsonArena());
    deserializeJson(doc, http.getStream());
    http.end();

    ESP_LOGI(TAG, "=== Spotify Devices ===");
    for (JsonObject d : doc["devices"].as<JsonArray>()) {
        ESP_LOGI(TAG, "  name=%-20s  id=%s  type=%s  active=%s",
            d["name"] | "?",
            d["id"]   | "?",
            d["type"] | "?",
            (d["is_active"] | false) ? "YES" : "no");
    }
    ESP_LOGI(TAG, "=======================");
}
