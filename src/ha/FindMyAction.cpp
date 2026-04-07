#include "FindMyAction.h"
#include "HAClient.h"
#include "../ui/ToastOverlay.h"
#include "../ui/fonts/fa_icons.h"

FindMyAction::FindMyAction(const char* account, const char* deviceName)
    : _account(account), _deviceName(deviceName) {}

void FindMyAction::trigger() {
    haClient.sendFindMyIPhone(_account, _deviceName);
    toast.show(FA_MOBILE, "Ringing...", 2000);
}
