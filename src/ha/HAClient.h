#pragma once

class HAClient {
public:
    void begin(const char* ssid, const char* password,
               const char* host, uint16_t port, const char* token);
    void update(); // call every loop(), non-blocking

private:
    void handleWifiConnecting();
    void handleWifiConnected();

    const char* _ssid;
    const char* _password;
    const char* _host;
    uint16_t    _port;
    const char* _token;

    uint32_t    _lastRetryMs = 0;
};

extern HAClient haClient;
