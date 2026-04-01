#pragma once
#include <lvgl.h>
#include "Screen.h"
#include "../AppState.h"

class ACControlScreen : public Screen {
public:
    void setACIndex(int idx);

    void init()               override;
    void show()               override;
    void onEncoder(int delta) override;
    void onButton()           override;
    void refresh()            override;
    void tick()               override;

private:
    enum class State { HERO, EDIT_TEMP, EDIT_MODE };

    void updateDisplay();
    void updateUnderline();
    int  findModeIdx(const char* mode, const ACState& ac);
    void resetActivityTimer();

    int  _acIdx       = 0;
    bool _initialized = false;

    State _state          = State::HERO;
    float _pendingTemp    = 0.0f;
    int   _pendingModeIdx = 0;

    lv_obj_t* _lvScreen;
    lv_obj_t* _arc;
    lv_obj_t* _arcInner;
    lv_obj_t* _arcMinLabel;
    lv_obj_t* _arcMaxLabel;
    lv_obj_t* _currentTempLabel;
    lv_obj_t* _targetTempLabel;
    lv_obj_t* _modeIconLabel;
    lv_obj_t* _modeTextLabel;
    lv_obj_t* _goBackLabel;
    lv_obj_t* _underline;

    int      _selected        = 0; // 0=target temp, 1=mode, 2=go back
    uint32_t _lastActivityMs  = 0;
    static constexpr uint32_t INACTIVITY_TIMEOUT_MS = 30000;
};
