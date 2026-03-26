#pragma once

class Screen {
public:
    virtual ~Screen() = default;
    virtual void init()             = 0;
    virtual void show()             = 0;
    virtual void onEncoder(int delta) = 0;
    virtual void onButton()         = 0;
    virtual void refresh()          {} // called when AppState dirty; override as needed
};
