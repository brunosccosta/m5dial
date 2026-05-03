#include "MathArtCard.h"
#include <math.h>
#include <string.h>
#include <esp_log.h>

static const char* TAG = "MATH_ART";

uint16_t* MathArtCard::pixBuf = nullptr;

void MathArtCard::init(lv_obj_t* parent) {
    if (!pixBuf) {
        pixBuf = (uint16_t*)malloc(W * H * 2);
        if (!pixBuf) {
            ESP_LOGE(TAG, "pixBuf alloc failed");
            return;
        }
        ESP_LOGI(TAG, "pixBuf allocated");
    }
    memset(pixBuf, 0, W * H * 2);

    // Create canvas here (before ring/overlay) so it sits below them in z-order
    _canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(_canvas, pixBuf, W, H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(_canvas, 0, 0);
    lv_obj_add_flag(_canvas, LV_OBJ_FLAG_HIDDEN);

    onInit(parent);
}

void MathArtCard::show() {
    if (!pixBuf) return;
    memset(pixBuf, 0, W * H * 2);
    lv_obj_clear_flag(_canvas, LV_OBJ_FLAG_HIDDEN);
    onShow();
}

void MathArtCard::hide() {
    onHide();
    lv_obj_add_flag(_canvas, LV_OBJ_FLAG_HIDDEN);
}

void MathArtCard::tick() {
    if (!_canvas) return;
    onTick();
}

void MathArtCard::invalidateCanvas() {
    if (_canvas) lv_obj_invalidate(_canvas);
}

uint16_t MathArtCard::rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

uint16_t MathArtCard::hsv565(float h, float s, float v) {
    float f  = fmodf(h / 60.0f, 6.0f);
    int   hi = (int)f;
    float ff = f - hi;
    float p  = v * (1.0f - s);
    float q  = v * (1.0f - ff * s);
    float t  = v * (1.0f - (1.0f - ff) * s);
    float r, g, b;
    switch (hi % 6) {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        default: r=v; g=p; b=q; break;
    }
    return rgb565((uint8_t)(r*255), (uint8_t)(g*255), (uint8_t)(b*255));
}
