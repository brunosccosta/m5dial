#pragma once
#include "../RestCard.h"

// Card 1 — current conditions: temperature + condition text.
// Icon slot reserved for FA glyph (deferred).
class WeatherNowCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;

private:
    lv_obj_t* _container;
    lv_obj_t* _tempLabel;
    lv_obj_t* _feelsLikeLabel;
    lv_obj_t* _iconLabel;
    lv_obj_t* _conditionLabel;
};
