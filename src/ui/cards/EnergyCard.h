#pragma once
#include <lvgl.h>
#include "../RestCard.h"

class EnergyCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;
    bool isVisible() const      override;

private:
    lv_obj_t* _container;
    lv_obj_t* _kwhLabel;        // big kWh number
    lv_obj_t* _kwhUnitLabel;    // "kWh\ntoday" inline to the right
    lv_obj_t* _boltIcon;        // row: bolt icon
    lv_obj_t* _wattsLabel;      // row: current watts
    lv_obj_t* _tariffLabel;     // row: €/kWh tariff
    lv_obj_t* _leafIcon;        // score row: leaf icon
    lv_obj_t* _scoreLabel;      // score row: "72 / 100"
};
