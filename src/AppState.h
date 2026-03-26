#pragma once
#include <Arduino.h>

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
};

extern AppState appState;
