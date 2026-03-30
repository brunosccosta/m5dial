#pragma once
#include <functional>
#include <lvgl.h>
#include "Screen.h"

class RestScreen : public Screen {
public:
    void setOnWake(std::function<void()> cb);

    void init()               override;
    void show()               override;
    void onEncoder(int delta) override;
    void onButton()           override;
    void refresh()            override;

private:
    void updateDisplay();
    void wake();

    bool _initialized = false;
    std::function<void()> _onWake;

    lv_obj_t* _lvScreen;

    // Current weather
    lv_obj_t* _currentTempLabel;
    lv_obj_t* _conditionLabel;

    // 1h forecast
    lv_obj_t* _forecastLabel;

    // Outside temp
    lv_obj_t* _outsideTempLabel;

    // AC status (left)
    lv_obj_t* _acIconLabel;
    lv_obj_t* _acStateLabel;

    // Heater status (right)
    lv_obj_t* _heaterIconLabel;
    lv_obj_t* _heaterStateLabel;
};

extern RestScreen restScreen;
