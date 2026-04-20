#pragma once
#include <stdint.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class SpotifyClient {
public:
    void begin();
    void update();

    bool play(const char* uri);
    bool transfer(const char* device_id);

    // Temporary: logs available Spotify Connect device IDs to Serial.
    // Remove once speaker puck IDs are captured.
    void logDevices();

private:
    bool refreshAccessToken();
    bool ensureToken();
    bool put(const char* path, const char* body);
    void beginHttp(HTTPClient& http, WiFiClientSecure& client, const char* url);

    char     _accessToken[300]  = {};
    char     _refreshToken[200] = {};
    uint32_t _tokenExpiresAt    = 0;   // millis() when token should be refreshed
    bool     _tokenValid        = false;
};

extern SpotifyClient spotifyClient;
