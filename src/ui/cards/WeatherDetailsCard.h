#pragma once
#include "../RestCard.h"

// Card 2 — weather details: feels like, outdoor temp (labelled), outdoor humidity.
class WeatherDetailsCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;

private:
    lv_obj_t* _container;
    lv_obj_t* _feelsLabel;
    lv_obj_t* _outdoorLabel;
    lv_obj_t* _humidLabel;
};
