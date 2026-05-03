#pragma once
#include <lvgl.h>
#include <stdint.h>
#include "../../RestCard.h"

// Base class for generative art rest cards.
// Owns a single shared 115KB pixel buffer (allocated once on first show).
// Subclasses implement onInit / onShow / onTick / onHide.
class MathArtCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update() override {}       // no AppState dependency
    void show()   override;
    void hide()   override;
    void tick()   override;
    bool isVisible() const override { return true; }

protected:
    // Canvas pixel buffer — shared across all MathArtCard subclasses
    static uint16_t* pixBuf;
    static const int W = 240, H = 240;

    lv_obj_t* _canvas = nullptr;

    void invalidateCanvas();

    static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
    static uint16_t hsv565(float h, float s, float v);

    virtual void onInit(lv_obj_t* parent) {}
    virtual void onShow() = 0;
    virtual void onTick() = 0;
    virtual void onHide() {}

};
