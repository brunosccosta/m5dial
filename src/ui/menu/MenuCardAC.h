#pragma once
#include <lvgl.h>
#include "MenuCard.h"
#include "../../AppState.h"
#include "../ACControlScreen.h"

// Menu preview card for AC / Heater. One instance per climate entity.
class MenuCardAC : public MenuCard {
public:
    MenuCardAC(const char* name, const char* iconGlyph,
               ACControlScreen& screen, ACState& ac);

    void        init(lv_obj_t* container) override;
    void        update()                  override;
    const char* label()                   override { return _name; }
    const char* icon()                    override { return _iconGlyph; }
    void        onSelect()                override;

private:
    const char*      _name;
    const char*      _iconGlyph;
    ACControlScreen& _screen;
    ACState&         _ac;

    lv_obj_t* _iconLabel;
    lv_obj_t* _nameLabel;
    lv_obj_t* _stateLabel;
};
