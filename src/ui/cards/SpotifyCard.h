#pragma once
#include "../RestCard.h"

class SpotifyCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;
    bool isVisible() const      override;

private:
    lv_obj_t* _container;
    lv_obj_t* _stateLabel;
    lv_obj_t* _titleLabel;
    lv_obj_t* _artistLabel;
    lv_obj_t* _shuffleLabel;  // hidden unless shuffle on
    lv_obj_t* _repeatLabel;   // hidden unless repeat on
    lv_obj_t* _volIconLabel;
    lv_obj_t* _volLabel;
    lv_obj_t* _srcIconLabel;
    lv_obj_t* _srcLabel;
};
