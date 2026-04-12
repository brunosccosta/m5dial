#pragma once
#include <stdint.h>

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

    char     _accessToken[300]  = {};
    char     _refreshToken[200] = {};
    uint32_t _tokenExpiresAt    = 0;   // millis() when token should be refreshed
    bool     _tokenValid        = false;
};

extern SpotifyClient spotifyClient;
