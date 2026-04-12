#pragma once

class RFIDHandler {
public:
    virtual const char* prefix() const = 0;
    virtual void handle(const char* uri) = 0;
    virtual ~RFIDHandler() = default;
};

class RFIDDispatcher {
public:
    void registerHandler(RFIDHandler* handler);
    void dispatch(const char* uri);

private:
    static constexpr int MAX_HANDLERS = 8;
    RFIDHandler* _handlers[MAX_HANDLERS] = {};
    int          _count = 0;
};

extern RFIDDispatcher rfidDispatcher;
