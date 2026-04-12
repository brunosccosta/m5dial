#pragma once
#include "RFIDDispatcher.h"

class SpotifyHandler : public RFIDHandler {
public:
    const char* prefix() const override { return "spotify:"; }
    void handle(const char* uri) override;
};

extern SpotifyHandler spotifyHandler;
