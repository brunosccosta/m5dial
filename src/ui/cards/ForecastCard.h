#pragma once
#include "../RestCard.h"

// Card 3 — 2-day forecast: today (1d) + tomorrow (2d) side by side.
// Shows condition icon, max temp, and rain chance per column.
class ForecastCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;

private:
    void updateColumn(int col);

    lv_obj_t* _container;

    lv_obj_t* _iconLeft;
    lv_obj_t* _tempLeft;
    lv_obj_t* _rainLeft;

    lv_obj_t* _iconRight;
    lv_obj_t* _tempRight;
    lv_obj_t* _rainRight;
};
