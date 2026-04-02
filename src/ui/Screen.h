#pragma once

class Screen {
public:
    virtual ~Screen() = default;
    virtual void init()             = 0;
    virtual void show()             = 0;
    virtual void onEncoder(int delta) = 0;
    virtual void onButton()           = 0;
    virtual void onTouch()            {} // called on tap (short press, no swipe)
    virtual void onSwipe(int dir)     {} // called on horizontal swipe; dir: +1=left, -1=right
    virtual void refresh()            {} // called when AppState dirty; override as needed
    virtual void tick()               {} // called every loop; override for timers/animations
};
