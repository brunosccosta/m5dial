#include "HAHandler.h"
#include "ha/HAClient.h"
#include "ui/ToastOverlay.h"
#include "ui/fonts/fa_icons.h"
#include <string.h>

HAHandler haHandler;

void HAHandler::handle(const char* uri) {
    const char* entity = uri + 3; // strip "ha:"
    haClient.sendTransferMedia(entity);

    // Extract room name (part after last '.') for the toast
    const char* dot = strrchr(entity, '.');
    toast.show(FA_TOWER_BROADCAST, dot ? dot + 1 : entity, 2500);
}
