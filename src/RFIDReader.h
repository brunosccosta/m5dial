#pragma once
#include <functional>
#include <stdint.h>

class RFIDReader {
public:
    using TagCallback = std::function<void(const char* uri)>;

    void begin();
    void update();
    void onTag(TagCallback cb) { _cb = cb; }

private:
    void readUltralight();

    TagCallback _cb;
    char        _uri[64]      = {};
    uint8_t     _lastUid[7]   = {};
    uint8_t     _lastUidSize  = 0;
    uint32_t    _lastReadMs   = 0;
};

extern RFIDReader rfidReader;
