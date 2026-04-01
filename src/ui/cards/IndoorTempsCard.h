#pragma once
#include <lvgl.h>
#include "../RestCard.h"

class IndoorTempsCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;

private:
    lv_obj_t* _container;

    // Balcony row
    lv_obj_t* _balconyIcon;
    lv_obj_t* _balconyTemp;
    lv_obj_t* _balconyDroplet;
    lv_obj_t* _balconyHumidity;

    // Bedroom row
    lv_obj_t* _bedroomIcon;
    lv_obj_t* _bedroomTemp;
    lv_obj_t* _bedroomDroplet;
    lv_obj_t* _bedroomHumidity;

    // Bathroom row
    lv_obj_t* _bathroomIcon;
    lv_obj_t* _bathroomTemp;
    lv_obj_t* _bathroomDroplet;
    lv_obj_t* _bathroomHumidity;

    void updateRow(lv_obj_t* tempLabel, lv_obj_t* humLabel, float temp, float humidity);
};
