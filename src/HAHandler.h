#pragma once
#include "RFIDDispatcher.h"

class HAHandler : public RFIDHandler {
public:
    const char* prefix() const override { return "ha:"; }
    void handle(const char* uri) override;
};

extern HAHandler haHandler;
