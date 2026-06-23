#pragma once
#include <lvgl.h>
#include "../RestCard.h"

// Hourly electricity price forecast (Zonneplan). Bar chart of the price from the
// current hour to the end of the published forecast (variable length, ~5–30h),
// each bar coloured by tariff group (low/normal/high → green/amber/red).
// Surfaces the upcoming evening spike at a glance.
class EnergyPriceCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;
    bool isVisible() const      override;

private:
    lv_obj_t* _container   = nullptr;
    lv_obj_t* _nowLabel    = nullptr; // current price hero "0.36"
    lv_obj_t* _unitLabel   = nullptr; // "€/kWh" inline
    lv_obj_t* _chart       = nullptr; // lv_chart bar
    lv_chart_series_t* _ser = nullptr;
    lv_obj_t* _peakLabel   = nullptr; // "Peak 1.10 @ 21:00"
};
