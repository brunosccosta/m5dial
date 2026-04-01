# Theme System — Implementation Plan

Centralise all UI colors into named constants. Multiple theme files selectable at compile time via a build flag — zero runtime cost, themes versioned in git.

---

## Structure

```
src/ui/
  Theme.h               ← dispatcher — the only file included by UI code
  themes/
    ThemeDefault.h      ← current grey/white/black palette (extracted from existing code)
    ThemeWarm.h         ← future experiments go here
    ...
```

`Theme.h` dispatches via preprocessor:

```cpp
#define THEME_DEFAULT 1
// add more as created: #define THEME_WARM 2

#ifndef ACTIVE_THEME
#define ACTIVE_THEME THEME_DEFAULT
#endif

#if ACTIVE_THEME == THEME_DEFAULT
#include "themes/ThemeDefault.h"
#endif
```

Switching theme = one line change in `platformio.ini`:
```ini
-DACTIVE_THEME=1   ; default
-DACTIVE_THEME=2   ; warm (when it exists)
```

---

## Color token inventory

Extracted from the 62 color references across UI files:

| Token | Current hex | Used for |
|---|---|---|
| `BG` | `0x000000` (`lv_color_black`) | Screen backgrounds |
| `TEXT_PRIMARY` | `0xFFFFFF` (`lv_color_white`) | Main labels, temps |
| `TEXT_DIM` | `0xAAAAAA` | Secondary labels (humidity, state) |
| `TEXT_FAINT` | `0x888888` | Icon tint, subtle labels |
| `TEXT_MUTED` | `0x666666` | Arc range labels, go-back, ring active |
| `SURFACE` | `0x333333` | Arc track background |
| `SURFACE_DARK` | `0x222222` | Inner arc track |
| `SURFACE_FAINT` | `0x444444` | Disabled arc indicator |
| `RING_ACTIVE` | `0x666666` | Timer ring indicator |
| `RING_BG` | `0x1E1E1E` | Timer ring background |
| `ACCENT_RAIN` | `0x5599CC` | Droplet icons + rain percentage |
| `TEMP_COLD` | `0x0088FF` | Cold temp arc (< 18°) |
| `TEMP_MID` | `0xFFCC00` | Transitional temp (18–21°) |
| `TEMP_WARM` | `0xFF6600` | Warm temp arc (≥ 21°) |
| `AC_MODE_OFF` | `0x333333` | Off mode |
| `AC_MODE_HEAT` | `0xFF4400` | Heat mode |
| `AC_MODE_COOL` | `0x0088FF` | Cool mode |
| `AC_MODE_AUTO` | `0x00BB44` | Auto mode |
| `AC_MODE_FAN` | `0xBBBBBB` | Fan mode |
| `AC_MODE_DRY` | `0xFFAA00` | Dry mode |
| `ACCENT_ERROR` | `0xFF3333` | Error overlay |

---

## Steps

1. Create `src/ui/themes/ThemeDefault.h` — all tokens as `constexpr uint32_t` in a `namespace Theme`
2. Create `src/ui/Theme.h` — dispatcher
3. Replace all hardcoded hex values across UI files with `Theme::TOKEN`
4. Add `-DACTIVE_THEME=1` to `platformio.ini`
5. Verify it compiles

All `lv_color_black()` and `lv_color_white()` calls are also replaced with `Theme::BG` and `Theme::TEXT_PRIMARY`.

**This PR is a pure refactor — no color values change.**

---

## Files to touch in step 3

- `src/ui/RestScreen.cpp`
- `src/ui/ACControlScreen.cpp`
- `src/ui/CarouselMenu.cpp`
- `src/ui/ErrorOverlay.cpp`
- `src/ui/LampControlScreen.cpp`
- `src/ui/cards/WeatherNowCard.cpp`
- `src/ui/cards/IndoorTempsCard.cpp`
- `src/ui/cards/ForecastCard.cpp`
