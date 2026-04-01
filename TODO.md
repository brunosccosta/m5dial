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
