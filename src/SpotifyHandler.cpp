#include "SpotifyHandler.h"
#include "SpotifyClient.h"
#include "ui/ToastOverlay.h"
#include "ui/fonts/fa_icons.h"
#include <string.h>

SpotifyHandler spotifyHandler;

void SpotifyHandler::handle(const char* uri) {
    bool ok;
    if (strncmp(uri, "spotify:device:", 15) == 0) {
        ok = spotifyClient.transfer(uri + 15); // strip "spotify:device:" prefix
        toast.show(ok ? FA_TOWER_BROADCAST : FA_BOLT, ok ? "Moving..." : "Failed", 2500);
    } else {
        ok = spotifyClient.play(uri);
        toast.show(ok ? FA_VOLUME_HIGH : FA_BOLT, ok ? "Playing" : "Failed", 2500);
    }
}
