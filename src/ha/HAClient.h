#pragma once
#include <WebSocketsClient.h>

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

    const char* _ssid;
    const char* _password;
    const char* _host;
    uint16_t    _port;
    const char* _token;

    uint32_t        _lastRetryMs = 0;
    WebSocketsClient _ws;
};

extern HAClient haClient;
