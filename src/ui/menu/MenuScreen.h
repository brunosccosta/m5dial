#pragma once
#include <lvgl.h>
#include "../Screen.h"
#include "MenuCard.h"

class MenuScreen : public Screen {
public:
    void addCard(MenuCard* card);

    void init()               override;
    void show()               override;
    void onEncoder(int delta) override;
    void onButton()           override;
    void onTouch()             override;
    void onSwipe(int dir)     override;
    void refresh()            override;
    void tick()               override;

    static constexpr int      MAX_CARDS        = 8;
    static constexpr int      ARC_GAP_DEG      = 5;
    static constexpr int      RING_WIDTH       = 3;
    static constexpr int      RING_START_ANGLE = 270;
    static constexpr int      SLIDE_ANIM_MS    = 200;
    static constexpr uint32_t INACTIVITY_MS    = 30000;

private:
    void buildArcSegments();
    void updateArcIndicator(int activeIdx);
    void slideTo(int newIdx, int direction); // direction: +1 = next, -1 = prev
    void resetActivityTimer();

    static void onSlideComplete(lv_anim_t* a);

    bool _initialized = false;
    bool _animating   = false;

    lv_obj_t* _lvScreen;
    lv_obj_t* _arcs[MAX_CARDS]       = {};
    lv_obj_t* _containers[MAX_CARDS] = {};

    MenuCard* _cards[MAX_CARDS] = {};
    int       _cardCount        = 0;
    int       _activeCard       = 0;
    uint32_t  _lastActivityMs   = 0;
};
