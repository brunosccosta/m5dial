#pragma once
#include <functional>
#include <lvgl.h>
#include "Screen.h"

// Generic confirmation screen. Push after calling setup().
// Touch on either option confirms immediately.
// Dial highlights an option; button confirms highlighted (defaults to No).
class ConfirmScreen : public Screen {
public:
    void setup(const char* question,
               std::function<void()> onYes,
               std::function<void()> onNo);

    void init()                override;
    void show()                override;
    lv_obj_t* lvScreen() const override { return _lvScreen; }
    void onEncoder(int delta)  override;
    void onButton()            override;
    void refresh()             override {}
    void tick()                override {}

private:
    void confirm(bool yes);
    void updateHighlight();

    static void onYesClick(lv_event_t* e);
    static void onNoClick(lv_event_t* e);

    const char*           _question  = nullptr;
    std::function<void()> _onYes;
    std::function<void()> _onNo;

    bool _initialized = false;
    int  _selected    = -1; // -1 = none, 0 = yes, 1 = no

    lv_obj_t* _lvScreen;
    lv_obj_t* _questionLabel;
    lv_obj_t* _yesContainer;
    lv_obj_t* _yesIconLabel;
    lv_obj_t* _yesTextLabel;
    lv_obj_t* _yesUnderline;
    lv_obj_t* _noContainer;
    lv_obj_t* _noIconLabel;
    lv_obj_t* _noTextLabel;
    lv_obj_t* _noUnderline;
};

extern ConfirmScreen confirmScreen;
