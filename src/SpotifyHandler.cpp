#include "SpotifyHandler.h"
#include "ha/HAClient.h"
#include "ui/ToastOverlay.h"
#include "ui/fonts/fa_icons.h"

SpotifyHandler spotifyHandler;

void SpotifyHandler::handle(const char* uri) {
    // TODO: use active speaker entity instead of hardcoded media_player.spotify
    haClient.sendPlayMedia("media_player.spotify", uri);
    toast.show(FA_VOLUME_HIGH, "Playing", 2500);
}
