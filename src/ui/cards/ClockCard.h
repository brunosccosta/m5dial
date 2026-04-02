#pragma once
#include <lvgl.h>
#include "../RestCard.h"

class ClockCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;

private:
    lv_obj_t* _container;
    lv_obj_t* _timeLabel;
    lv_obj_t* _dateLabel;
};
