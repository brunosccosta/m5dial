#pragma once

class FindMyAction {
public:
    FindMyAction(const char* account, const char* deviceName);
    void trigger();

private:
    const char* _account;
    const char* _deviceName;
};
