#pragma once
#include <lvgl.h>

// ErrorOverlay renders on lv_layer_top() — always above all screens.
// Currently driven by appState.connection; future: driven by an error
// queue in AppState so any subsystem (HAClient, sensors, etc.) can push errors.

class ErrorOverlay {
public:
    void init();
    void update(); // call every loop(); reads appState.connection

    // Public so static fade helpers in .cpp can reference them
    static void pulseAnimCb(void* var, int32_t val);
    static void fadeAnimCb(void* var, int32_t val);

private:
    enum class State { HIDDEN, FULL, DOT };

    void showFull(const char* icon, const char* message);
    void collapseToDot();
    void hideAll();

    State    _state       = State::HIDDEN;
    uint32_t _shownAt     = 0;
    bool     _initialized = false;

    lv_obj_t* _bg;
    lv_obj_t* _icon;
    lv_obj_t* _label;
    lv_obj_t* _dot;

    static constexpr uint32_t COLLAPSE_MS = 4000;
};

extern ErrorOverlay errorOverlay;
