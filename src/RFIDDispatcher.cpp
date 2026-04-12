#include "RFIDDispatcher.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "RFID";

RFIDDispatcher rfidDispatcher;

void RFIDDispatcher::registerHandler(RFIDHandler* handler) {
    if (_count < MAX_HANDLERS) _handlers[_count++] = handler;
}

void RFIDDispatcher::dispatch(const char* uri) {
    for (int i = 0; i < _count; i++) {
        const char* pfx = _handlers[i]->prefix();
        if (strncmp(uri, pfx, strlen(pfx)) == 0) {
            _handlers[i]->handle(uri);
            return;
        }
    }
    ESP_LOGW(TAG, "no handler for: %s", uri);
}
