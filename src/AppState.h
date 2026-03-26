#pragma once
#include <Arduino.h>

namespace ErrorKey {
    constexpr const char* WIFI  = "wifi";
    constexpr const char* HA_WS = "ha_ws";
}

struct ErrorEntry {
    char     key[20];
    uint32_t registeredAt;
    uint32_t fireAfterMs;
    bool     active;
};

enum class ConnectionState {
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    HA_CONNECTING,
    HA_READY,
};

struct LampState {
    const char* name;
    bool        on;
    uint8_t     brightness; // 0-255
};

struct ACState {
    const char* name;
    float       current_temp;
    float       target_temp;
    String      mode; // "cool", "heat", "auto", "off"
};

struct AppState {
    LampState lamps[4];
    int       lampCount;

    ACState   acs[2];
    int       acCount;

    float     room_temp;

    bool            dirty;      // set by HA layer when state changes; cleared by UI after refresh
    ConnectionState connection; // written by HAClient; read by UI for status indicator

    static constexpr int MAX_ERRORS = 6;
    ErrorEntry errors[MAX_ERRORS];

    void setError(const char* key, uint32_t fireAfterMs = 0);
    void clearError(const char* key);
};

extern AppState appState;
