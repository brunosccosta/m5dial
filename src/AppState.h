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

struct ACState {
    const char* entity_id;
    const char* name;
    float       current_temp;
    float       target_temp;
    char        mode[12];             // "off", "cool", "heat", "auto", "fan_only", "dry"
    char        availableModes[6][12]; // populated from hvac_modes on first HA update
    int         modeCount;
    bool        valid;                // false until first HA update arrives
};

struct WeatherState {
    char  condition[32];         // weather.buienradar state e.g. "sunny"
    char  detailedCondition[32]; // sensor.detailed_condition e.g. "partlycloudy-rain"
    float temperature;           // weather.buienradar a.temperature
    float feelsLike;             // weather.buienradar a.apparent_temperature
    bool  isDaytime;             // sun.sun: true = above_horizon, false = below_horizon
    bool  valid;                 // false until first HA update arrives
};

struct SensorState {
    float outdoorTemp;      // sensor.atc_3294_temperature
    float outdoorHumidity;  // sensor.atc_3294_humidity
    float bedroomTemp;      // sensor.atc_03be_temperature
    float bedroomHumidity;  // sensor.atc_03be_humidity
    float bathroomTemp;     // sensor.atc_88dc_temperature
    float bathroomHumidity; // sensor.atc_88dc_humidity
};

struct SpotifyState {
    char  state[12];  // "playing" / "paused" / "idle" / "off"
    char  title[64];
    char  artist[64];
    char  source[32]; // "iPhone" / "Sala" / "Living Room"
    float volume;     // 0.0–1.0
    bool  shuffle;
    char  repeat[8];  // "off" / "one" / "all"
    bool  valid;      // false until first HA update
};

struct ForecastDay {
    char  detailedCondition[32]; // sensor.detailed_condition_1d / _2d
    float temperature;           // sensor.temperature_1d / _2d (max)
    int   rainChance;            // sensor.rainchance_1d / _2d (0–100)
    bool  valid;                 // false until first HA update arrives
};

struct AppState {
    ACState      ac;               // climate.forninho_room_temperature
    ACState      heater;           // climate.forninho_portatil
    WeatherState weather;
    SensorState  sensors;
    ForecastDay  forecastToday;    // sensor.*_1d
    ForecastDay  forecastTomorrow; // sensor.*_2d
    SpotifyState spotify;          // media_player.spotify

    bool            dirty;      // set by HA layer when state changes; cleared by UI after refresh
    ConnectionState connection; // written by HAClient; read by UI for status indicator

    static constexpr int MAX_ERRORS = 6;
    ErrorEntry errors[MAX_ERRORS];

    void setError(const char* key, uint32_t fireAfterMs = 0);
    void clearError(const char* key);
};

extern AppState appState;
