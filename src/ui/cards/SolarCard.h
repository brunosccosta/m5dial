#pragma once
#include <lvgl.h>
#include "../RestCard.h"

class SolarCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;
    bool isVisible() const      override;

private:
    lv_obj_t* _container;
    lv_obj_t* _kwhLabel;        // hero: today's kWh
    lv_obj_t* _kwhUnitLabel;    // "kWh\ntoday"
    lv_obj_t* _sunIcon;         // row 2: sun icon
    lv_obj_t* _liveWLabel;      // row 2: live watts
    lv_obj_t* _boltIcon;        // row 3: bolt icon
    lv_obj_t* _valueEurLabel;   // row 3: solar value EUR
    lv_obj_t* _battIcon;        // row 4: battery icon
    lv_obj_t* _savingsEurLabel; // row 4: battery savings EUR
};
