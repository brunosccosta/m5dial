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

AppState appState = {
    .loveMode   = true,
    .dirty      = false,
    .connection = ConnectionState::WIFI_CONNECTING,
};
