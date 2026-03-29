#pragma once
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

class HAClient {
public:
    void begin(const char* ssid, const char* password,
               const char* host, uint16_t port, const char* token);
    void update(); // call every loop(), non-blocking

    void onWsEvent(WStype_t type, uint8_t* payload, size_t length);

private:
    void handleWifiConnecting();
    void handleWifiConnected();
    void handleHaConnecting();
    void handleHaReady();

    void connectWebSocket();
    void handleMessage(uint8_t* payload, size_t length);
    void sendAuth();
    void subscribeEntities();
    void updateACState(const char* entity_id, const char* state, JsonObject attrs);

    const char* _ssid;
    const char* _password;
    const char* _host;
    uint16_t    _port;
    const char* _token;

    uint32_t         _lastRetryMs = 0;
    WebSocketsClient _ws;

    uint16_t _msgId       = 0;
    uint16_t _subscribeId = 0;
};

extern HAClient haClient;
