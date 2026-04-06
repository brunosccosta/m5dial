#pragma once
#include <functional>
#include <lvgl.h>
#include "Screen.h"
#include "RestCard.h"
#include "cards/ClockCard.h"
#include "cards/WeatherNowCard.h"
#include "cards/IndoorTempsCard.h"
#include "cards/ForecastCard.h"
#include "cards/SpotifyCard.h"
#include "cards/MeshCoreCard.h"

class RestScreen;  // forward declaration for FooterSlot

struct FooterSlot {
    lv_obj_t*             iconLabel  = nullptr;
    lv_obj_t*             stateLabel = nullptr;
    lv_obj_t*             touchArea  = nullptr;
    std::function<void()> onTap;
    RestScreen*           owner      = nullptr;
};

class RestScreen : public Screen {
public:
    void setOnWake(std::function<void()> cb);
    void setFooterTap(int side, std::function<void()> cb);

    void init()               override;
    void show()               override;
    void onEncoder(int delta) override;
    void onButton()           override;
    void onTouch()            override;
    void refresh()            override;
    void tick()               override;

    // Milliseconds per card. Adjust to taste.
    static constexpr uint32_t CARD_INTERVAL_MS = 60000;

    // Pin to a specific card index during development (-1 = normal rotation).
    // 0 = ClockCard, 1 = WeatherNowCard, 2 = IndoorTempsCard, 3 = ForecastCard, 4 = SpotifyCard, 5 = MeshCoreCard
    static constexpr int DEV_CARD_PIN = -1;

    // Timer ring — thin arc around the dial edge showing time until next card.
    static constexpr int RING_WIDTH       = 3;
    static constexpr int RING_START_ANGLE = 270;  // 270 = 12 o'clock

private:
    void updateDisplay();
    void updateDeviceStrip();
    void navigateCard(int dir);  // +1 = forward, -1 = backward
    void advanceCard();
    void wake();

    static void onFooterTap(lv_event_t* e);

    static constexpr int MAX_CARDS = 8;

    bool _initialized        = false;
    bool _footerTapConsumed  = false;
    std::function<void()> _onWake;

    lv_obj_t* _lvScreen;

    // Card instances — add new cards here
    ClockCard        _cardClock;
    WeatherNowCard   _cardWeatherNow;
    IndoorTempsCard  _cardIndoorTemps;
    ForecastCard     _cardForecast;
    SpotifyCard      _cardSpotify;
    MeshCoreCard     _cardMeshCore;

    // Card registry — populated in init()
    RestCard* _cards[MAX_CARDS] = {};
    int       _cardCount        = 0;
    int       _activeCard       = 0;
    uint32_t  _lastAdvanceMs    = 0;
    uint32_t  _lastCardUpdateMs = 0;

    // Timer ring
    lv_obj_t* _ring;
    int       _lastRingValue = -1;

    // Device strip (always visible, not part of card rotation)
    FooterSlot _slots[2];
};

extern RestScreen restScreen;
