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
    void sendFindMyIPhone(const char* account, const char* deviceName);
    void sendPlayMedia(const char* entity_id, const char* uri);
    void sendTransferMedia(const char* target_entity_id);
    void requestKwhBaseline(); // public so parseEnergyUsageKwh can trigger day-rollover re-query

    void onWsEvent(WStype_t type, uint8_t* payload, size_t length);

    uint32_t wsReconnects() const { return _wsReconnects; }

private:
    void handleWifiConnecting();
    void handleWifiConnected();
    void handleHaConnecting();
    void handleHaReady();

    void connectWebSocket();
    void handleMessage(uint8_t* payload, size_t length);
    void sendAuth();
    void subscribeEntities();
    void requestBatteryHistory();
    void dispatchSensor(const char* entity_id, const char* state, JsonObject attrs);

    static constexpr int BATCH_SIZE    = 5;

    const char* _ssid;
    const char* _password;
    const char* _host;
    uint16_t    _port;
    const char* _token;

    bool             _dryRun      = false;
    uint32_t         _lastRetryMs = 0;
    WebSocketsClient _ws;

    uint16_t _msgId              = 0;
    int      _subscribeCount     = 0; // number of subscribe_entities batches sent (ids 1.._subscribeCount)
    uint16_t _batteryHistoryMsgId = 0;
    uint16_t _kwhBaselineMsgId    = 0;
    uint32_t _wsReconnects        = 0;
};

extern HAClient haClient;
