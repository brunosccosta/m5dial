# TODO

Small improvements and known issues parked for later.

---

## Navigation — menu item registry

**File:** `src/main.cpp` — `setupNavigation()`

**Problem:** Adding a menu item currently requires touching three places: the CarouselMenu item list, the `setOnSelect` switch, and the screen wiring. Same friction as the old sensor setup.

**Decision:** Flat `MENU_ITEMS[]` registry, same pattern as `SENSORS[]`. Each entry drives display (icon + label) and behaviour (which screen to push + optional setup step).

```cpp
struct MenuItem {
    const char*            icon;
    const char*            label;
    Screen*                screen;
    std::function<void()>  setup;   // called before push; nullptr if none
};

static MenuItem MAIN_MENU[] = {
    { FA_LIGHTBULB, "Lamps",   &lampMenu,   nullptr                                  },
    { FA_WIND,      "AC",      &acControl,  []{ acControl.setAC(&appState.ac);     } },
    { FA_FIRE,      "Heater",  &acControl,  []{ acControl.setAC(&appState.heater); } },
};
```

CarouselMenu is populated by looping `MAIN_MENU`. `setOnSelect` becomes a single loop: call `setup()` if set, then `screenManager.push(screen)`.

**Notes:**
- `std::function<void()>` preferred over raw function pointer — lambda ergonomics, ESP32 heap headroom is fine
- Nested lamp submenu stays as-is for now — registry covers main menu only
- Adding a new device = one row in `MAIN_MENU` + implement the screen

---

## Font tooling — bash script for regeneration

Currently adding a glyph requires manually editing the `--range` list and running three separate `lv_font_conv` commands (32px, 24px, 18px). Easy to get them out of sync.

**Fix:** A single `tools/gen_fonts.sh` script that owns the canonical glyph list and regenerates all sizes in one shot:

```bash
#!/usr/bin/env bash
GLYPHS="61675,62172,61549,..."   # single source of truth
lv_font_conv --bpp 4 --size 32 --range $GLYPHS ... -o src/ui/fonts/font_awesome_solid_32.c
lv_font_conv --bpp 4 --size 24 --range $GLYPHS ... -o src/ui/fonts/font_awesome_solid_24.c
lv_font_conv --bpp 4 --size 18 --range $GLYPHS ... -o src/ui/fonts/font_awesome_solid_18.c
```

Adding a glyph = add one decimal codepoint to `GLYPHS`, run the script, add the `#define` to `fa_icons.h`. Update `fonts/README.md` to point to the script instead of the raw commands.

---

## Room temperatures should not live on WeatherState

`bedroomTemp`, `bedroomHumidity`, `bathroomTemp`, `bathroomHumidity` (and arguably `outdoorTemp`/`outdoorHumidity`) are sensor readings from ATC devices, not weather data. They ended up on `WeatherState` for convenience but that's the wrong abstraction. Should be a separate struct (e.g. `SensorTemps` or individual named fields at the `AppState` level).

---

## Buzzer feedback

M5Dial has an 80dB buzzer via `M5Dial.Speaker`. Potential uses: confirmation beep on button press, alert on sensor threshold (e.g. room too hot), error sound on WiFi/HA disconnect, encoder tick tone. Investigate what feels useful vs. annoying.

---

## Clock card (NTP-based)

Add a RestScreen card showing current time (and optionally date). Sync via NTP on boot using `configTime()` — no HA entity needed, no RTC dependency. Read time in `update()` via `time()` / `localtime()`. Consider time zone config in `credentials.h` or `Config.h`.

---

## Investigate touch-based controls

M5Dial has a capacitive touch screen (FT3267, confirmed working). Touch coordinates are available via `M5Dial.Touch.getDetail()`. Investigate usability patterns: tap to select, swipe to scroll cards, tap zones on the round display, or hybrid encoder+touch interactions.

---

## Screen transition animations

Smooth slide or fade between screens on push/pop. LVGL has `lv_scr_load_anim()` for this — replace the bare `lv_scr_load()` calls in each screen's `show()`. Decide on animation style (fade, slide left/right) and duration.

---
