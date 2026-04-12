#include "RFIDReader.h"
#include <M5Dial.h>
#include <esp_log.h>
#include <string.h>

static const char* TAG = "RFID";

RFIDReader rfidReader;

void RFIDReader::begin() {
    // RFID hardware init is handled by M5Dial.begin(cfg, encoder, true)
}

void RFIDReader::update() {
    if (!M5Dial.Rfid.PICC_IsNewCardPresent()) return;
    if (!M5Dial.Rfid.PICC_ReadCardSerial())   return;

    auto& rfid = M5Dial.Rfid;

    // Debounce: same UID within 2s → ignore
    uint32_t now = millis();
    bool sameUid = (rfid.uid.size == _lastUidSize) &&
                   (memcmp(rfid.uid.uidByte, _lastUid, rfid.uid.size) == 0);
    if (sameUid && (now - _lastReadMs < 2000)) {
        rfid.PICC_HaltA();
        return;
    }
    memcpy(_lastUid, rfid.uid.uidByte, rfid.uid.size);
    _lastUidSize = rfid.uid.size;
    _lastReadMs  = now;

    if (rfid.PICC_GetType(rfid.uid.sak) == MFRC522::PICC_TYPE_MIFARE_UL) {
        readUltralight();
    } else {
        rfid.PICC_HaltA();
    }
}

void RFIDReader::readUltralight() {
    auto& rfid = M5Dial.Rfid;

    // Read pages 4–15 (user memory, 12 pages = 48 bytes)
    // MIFARE_Read reads 4 pages (16 bytes) per call
    uint8_t data[48];
    for (uint8_t startPage = 4; startPage < 16; startPage += 4) {
        uint8_t buf[18]; uint8_t sz = 18;
        if (rfid.MIFARE_Read(startPage, buf, &sz) != MFRC522::STATUS_OK) {
            ESP_LOGW(TAG, "read failed at page %d", startPage);
            rfid.PICC_HaltA();
            return;
        }
        memcpy(data + (startPage - 4) * 4, buf, 16);
    }
    rfid.PICC_HaltA();

    // TLV walk (NTAG213 user memory layout)
    int i = 0;
    while (i < (int)sizeof(data) - 1) {
        uint8_t tlvType = data[i++];
        if (tlvType == 0xFE) break;    // terminator TLV
        if (tlvType == 0x00) continue; // null TLV, skip

        // Length field: 1 byte, or 3 bytes if first byte is 0xFF
        int tlvLen;
        if (data[i] == 0xFF) {
            if (i + 3 > (int)sizeof(data)) break;
            tlvLen = ((int)data[i+1] << 8) | data[i+2];
            i += 3;
        } else {
            tlvLen = data[i++];
        }

        if (tlvType != 0x03) { i += tlvLen; continue; } // skip non-NDEF TLVs

        // NDEF record header
        if (i + 3 > (int)sizeof(data)) break;
        uint8_t flags   = data[i++];
        bool    sr      = flags & 0x10; // short record: payload length is 1 byte
        bool    il      = flags & 0x08; // ID length field present
        uint8_t typeLen = data[i++];

        uint32_t payloadLen;
        if (sr) {
            payloadLen = data[i++];
        } else {
            if (i + 4 > (int)sizeof(data)) break;
            payloadLen = ((uint32_t)data[i]   << 24) | ((uint32_t)data[i+1] << 16) |
                         ((uint32_t)data[i+2] <<  8) |  (uint32_t)data[i+3];
            i += 4;
        }
        if (il) i++; // skip ID length byte

        // Only handle Well-Known Text records (type 'T')
        if (typeLen != 1 || i >= (int)sizeof(data) || data[i] != 'T') {
            ESP_LOGW(TAG, "not a Text record, skipping");
            return;
        }
        i += typeLen; // skip type byte

        // Text record payload: status byte + lang + text
        if (i >= (int)sizeof(data) || payloadLen < 1) break;
        uint8_t status  = data[i++];
        uint8_t langLen = status & 0x3F;
        i += langLen; // skip lang

        int textLen = (int)payloadLen - 1 - (int)langLen;
        if (textLen <= 0 || i >= (int)sizeof(data)) break;
        if (textLen > (int)(sizeof(_uri) - 1)) textLen = sizeof(_uri) - 1;

        memcpy(_uri, data + i, textLen);
        _uri[textLen] = '\0';

        ESP_LOGI(TAG, "tag: %s", _uri);
        if (_cb) _cb(_uri);
        return;
    }

    ESP_LOGW(TAG, "no NDEF Text record found");
}
