#pragma once
#include <functional>
#include <lvgl.h>
#include "Screen.h"
#include "RestCard.h"
#include "cards/WeatherNowCard.h"
#include "cards/WeatherDetailsCard.h"
#include "cards/ForecastCard.h"

class RestScreen : public Screen {
public:
    void setOnWake(std::function<void()> cb);

    void init()               override;
    void show()               override;
    void onEncoder(int delta) override;
    void onButton()           override;
    void refresh()            override;
    void tick()               override;

    // Milliseconds per card. Adjust to taste.
    static constexpr uint32_t CARD_INTERVAL_MS = 60000;

    // Pin to a specific card index during development (-1 = normal rotation).
    // 0 = WeatherNowCard, 1 = WeatherDetailsCard, etc.
    static constexpr int DEV_CARD_PIN = -1;

    // Timer ring — thin arc around the dial edge showing time until next card.
    static constexpr int      RING_WIDTH        = 3;
    static constexpr uint32_t RING_ACTIVE_COLOR = 0x666666;
    static constexpr uint32_t RING_BG_COLOR     = 0x1E1E1E;
    static constexpr int      RING_START_ANGLE  = 270;  // 270 = 12 o'clock

private:
    void updateDisplay();
    void updateDeviceStrip();
    void advanceCard();
    void wake();

    static constexpr int MAX_CARDS = 8;

    bool _initialized = false;
    std::function<void()> _onWake;

    lv_obj_t* _lvScreen;

    // Card instances — add new cards here
    WeatherNowCard     _cardWeatherNow;
    WeatherDetailsCard _cardWeatherDetails;
    ForecastCard       _cardForecast;

    // Card registry — populated in init()
    RestCard* _cards[MAX_CARDS] = {};
    int       _cardCount        = 0;
    int       _activeCard       = 0;
    uint32_t  _lastAdvanceMs    = 0;

    // Timer ring
    lv_obj_t* _ring;
    int       _lastRingValue = -1;

    // Device strip (always visible, not part of card rotation)
    lv_obj_t* _acIconLabel;
    lv_obj_t* _acStateLabel;
    lv_obj_t* _heaterIconLabel;
    lv_obj_t* _heaterStateLabel;
};

extern RestScreen restScreen;
