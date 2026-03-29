#include <string.h>
#include <esp_log.h>
#include "AppState.h"

void AppState::setError(const char* key, uint32_t fireAfterMs) {
    for (int i = 0; i < MAX_ERRORS; i++) {
        if (errors[i].active && strncmp(errors[i].key, key, sizeof(errors[i].key)) == 0)
            return; // already active, keep timer running
    }
    for (int i = 0; i < MAX_ERRORS; i++) {
        if (!errors[i].active) {
            strncpy(errors[i].key, key, sizeof(errors[i].key) - 1);
            errors[i].key[sizeof(errors[i].key) - 1] = '\0';
            errors[i].registeredAt = millis();
            errors[i].fireAfterMs  = fireAfterMs;
            errors[i].active       = true;
            return;
        }
    }
    ESP_LOGW("AppState", "error registry full, dropping: %s", key);
}

void AppState::clearError(const char* key) {
    for (int i = 0; i < MAX_ERRORS; i++) {
        if (errors[i].active && strncmp(errors[i].key, key, sizeof(errors[i].key)) == 0) {
            errors[i].active = false;
            return;
        }
    }
}

void AppState::initDevices() {
    for (int i = 0; i < AC_COUNT; i++) {
        acs[i].entity_id    = ACS[i].entity_id;
        acs[i].name         = ACS[i].name;
        acs[i].current_temp = 0.0f;
        acs[i].target_temp  = 0.0f;
        strncpy(acs[i].mode, "off", sizeof(acs[i].mode));
        acs[i].valid        = false;
    }
}

AppState appState = {
    .lamps = {
        {"Living Room", false, 0},
        {"Bedroom",     true,  200},
        {"Kitchen",     false, 0},
        {"Office",      true,  128},
    },
    .lampCount  = 4,
    .room_temp  = 0.0f,
    .dirty      = false,
    .connection = ConnectionState::WIFI_CONNECTING,
};
