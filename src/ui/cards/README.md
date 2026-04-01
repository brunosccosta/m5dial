# RestScreen Cards

Each card is one rotating "page" in the upper area of RestScreen. The device strip (AC + heater) is always visible below and is not part of the card system.

---

## Adding a new card

### 1. Create the card files

`src/ui/cards/MyNewCard.h`:
```cpp
#pragma once
#include "../RestCard.h"

class MyNewCard : public RestCard {
public:
    void init(lv_obj_t* parent) override;
    void update()               override;
    void show()                 override;
    void hide()                 override;

private:
    lv_obj_t* _container;
    lv_obj_t* _someLabel;
    // add more labels as needed
};
```

`src/ui/cards/MyNewCard.cpp`:
```cpp
#include "MyNewCard.h"
#include "../../AppState.h"

void MyNewCard::init(lv_obj_t* parent) {
    // Always start with this boilerplate container:
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    // Create your labels as children of _container
    _someLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_someLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_someLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(_someLabel, LV_ALIGN_CENTER, 0, -35);
}

void MyNewCard::update() {
    // Pull from appState, format with snprintf, set text
    // Re-align after text update (text size may change width)
    lv_label_set_text(_someLabel, "hello");
    lv_obj_align(_someLabel, LV_ALIGN_CENTER, 0, -35);
}

void MyNewCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); }
void MyNewCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
```

### 2. Register in RestScreen.h

Add a member instance alongside the other cards:
```cpp
// Card instances — add new cards here
WeatherNowCard     _cardWeatherNow;
WeatherDetailsCard _cardWeatherDetails;
MyNewCard          _cardMyNew;          // ← add this
```

### 3. Register in RestScreen.cpp `init()`

Add one line in the registration block:
```cpp
_cards[_cardCount++] = &_cardWeatherNow;
_cards[_cardCount++] = &_cardWeatherDetails;
_cards[_cardCount++] = &_cardMyNew;     // ← add this
```

That's it. No other changes needed.

### Dev tip: pin to a card while developing

Set `DEV_CARD_PIN` in `RestScreen.h` to the index of the card you're working on — rotation stops and the screen opens on that card directly. Set back to `-1` when done.

```cpp
static constexpr int DEV_CARD_PIN = 2; // 0 = WeatherNow, 1 = WeatherDetails, etc.
```

---

## Layout constraints

The display is a **240×240 circle**. All coordinates below are offsets from center (0,0).

```
        y = -100  ← approx top of safe card area (circle clips beyond ~±85px from center)
             ...
        y = -35   ← visual midpoint of card area — aim for content centered here
             ...
        y = +15   ← FLOOR: do not place card content below this line
                     (WeatherDetailsCard's bottom row sits here — verified on device)
        y = +30   ─── separator (implicit, no visual divider)
        y = +42   ← device strip icons start here (not for cards)
        y = +60   ← device strip text
        y = +80   ← circle edge clips here at center x
```

**Rules:**
- Keep all card label y values between **-80** and **+15**
- The card area visual center is **y ≈ -35** — vertically center your content around this
- Don't go wider than ~±100px at y=0; the circle clips at ±120px from center at all y values
- Use `snprintf` + `lv_label_set_text` for any float values — `lv_label_set_text_fmt` does not support `%f`
- Re-call `lv_obj_align` in `update()` after changing label text (text width changes shift the position)

---

## Existing cards

| File | Content | y range |
|---|---|---|
| `WeatherNowCard` | Temp 48px + feels-like 14px / icon + condition 24px | -45 to -8 |
| `IndoorTempsCard` | 3 rows: balcony/bedroom/bathroom — icon + temp 24px + droplet + humidity | -58 to +8 |
| `ForecastCard` | 2-column: TODAY / TMR — icon + max temp + rain % | -72 to -8 |
