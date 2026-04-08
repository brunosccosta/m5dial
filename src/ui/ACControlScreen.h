#pragma once
#include <lvgl.h>
#include "Screen.h"
#include "../AppState.h"

class ACControlScreen : public Screen {
public:
    void setAC(ACState* ac);

    void init()                override;
    void show()                override;
    lv_obj_t* lvScreen() const override { return _lvScreen; }
    void onEncoder(int delta)  override;
    void onButton()           override;
    void refresh()            override;
    void tick()               override;

private:
    bool hasChanges() const;
    void updateDisplay();
    void toggleMode();

    static void onModeClick(lv_event_t* e);

    ACState* _ac          = nullptr;
    bool     _initialized = false;

    float _pendingTemp = 0.0f;
    char  _pendingMode[12] = {};
    float _originalTemp = 0.0f;
    char  _originalMode[12] = {};

    uint32_t _lastActivityMs = 0;
    static constexpr uint32_t INACTIVITY_MS = 30000;

    lv_obj_t* _lvScreen;
    lv_obj_t* _arc;
    lv_obj_t* _arcInner;
    lv_obj_t* _currentTempLabel;
    lv_obj_t* _targetTempLabel;
    lv_obj_t* _modeContainer;
    lv_obj_t* _modeIconLabel;
    lv_obj_t* _modeTextLabel;
};
