#pragma once
#include <lvgl.h>
#include "../RestCard.h"

class FlightCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;
    bool isVisible()      const override;

private:
    static int  daysUntil(int year, int month, int day);
    static void countdownColor(int days, lv_color_t& out);

    lv_obj_t* _container  = nullptr;
    lv_obj_t* _flagImg    = nullptr;
    lv_obj_t* _destLabel  = nullptr;
    lv_obj_t* _countLabel = nullptr;
    lv_obj_t* _dateLabel  = nullptr;
    lv_obj_t* _hintLabel  = nullptr;

    bool _initialized = false;

    static constexpr uint32_t COLOR_NORMAL = 0xFFFFFF;
    static constexpr uint32_t COLOR_WARN   = 0xFFAA00;
    static constexpr uint32_t COLOR_URGENT = 0xFF3333;
};
