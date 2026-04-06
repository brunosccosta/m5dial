#pragma once
#include <lvgl.h>
#include "../RestCard.h"

class MeshCoreCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;

private:
    lv_obj_t* _container;

    // Header
    lv_obj_t* _headerIcon;
    lv_obj_t* _headerName;

    // Status
    lv_obj_t* _statusDot;
    lv_obj_t* _statusLabel;

    // Stats
    lv_obj_t* _batIcon;
    lv_obj_t* _batLabel;
    lv_obj_t* _batTrendIcon;
    lv_obj_t* _batDiffLabel;
    lv_obj_t* _uptimeIcon;
    lv_obj_t* _uptimeLabel;
};
