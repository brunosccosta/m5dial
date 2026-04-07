#pragma once
#include <lvgl.h>
#include "MenuCard.h"
#include "../../ha/FindMyAction.h"

class MenuCardFindMy : public MenuCard {
public:
    MenuCardFindMy(const char* account, const char* deviceName);

    void        init(lv_obj_t* container) override;
    void        update()                  override {}
    const char* label()                   override { return "Find iPhone"; }
    const char* icon()                    override;
    void        onSelect()                override;

private:
    FindMyAction _action;

    lv_obj_t* _iconLabel;
    lv_obj_t* _nameLabel;
};
