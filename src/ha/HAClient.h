#pragma once
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

class HAClient {
public:
    void begin(const char* ssid, const char* password,
               const char* host, uint16_t port, const char* token,
               bool dryRun = false);
    void update(); // call every loop(), non-blocking

    void sendACTemperature(const char* entity_id, float temp);
    void sendACMode(const char* entity_id, const char* mode);

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
    void dispatchSensor(const char* entity_id, const char* state, JsonObject attrs);

    static constexpr int BATCH_SIZE    = 5;
    static constexpr int MAX_BATCHES   = 4; // ceil(SENSOR_COUNT / BATCH_SIZE)

    const char* _ssid;
    const char* _password;
    const char* _host;
    uint16_t    _port;
    const char* _token;

    bool             _dryRun      = false;
    uint32_t         _lastRetryMs = 0;
    WebSocketsClient _ws;

    uint16_t _msgId                    = 0;
    uint16_t _subscribeIds[MAX_BATCHES] = {};
    int      _subscribeCount            = 0;
};

extern HAClient haClient;
