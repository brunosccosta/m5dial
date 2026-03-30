#pragma once
#include "Screen.h"

class ScreenManager {
public:
    void push(Screen* screen);
    void pop();
    void onEncoder(int delta);
    void onButton();
    void refresh(); // forward to active screen when AppState dirty
    void tick();    // forward to active screen every loop

private:
    static constexpr int MAX_DEPTH = 4;
    Screen* _stack[MAX_DEPTH] = {};
    int     _depth = 0;
};

extern ScreenManager screenManager;
