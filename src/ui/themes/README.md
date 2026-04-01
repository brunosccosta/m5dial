# UI Themes

Compile-time color theming. All UI color constants live here — one theme active at a time, selected via a build flag.

---

## How it works

`Theme.h` (one level up) is the only file UI code includes. It dispatches to the active theme via preprocessor:

```cpp
#include "Theme.h"   // ← always this, never a theme file directly
```

The active theme is set in `platformio.ini`:
```ini
-DACTIVE_THEME=1   ; 1 = ThemeDefault
```

---

## Adding a new theme

1. Copy `ThemeDefault.h` → `ThemeWarm.h` (or whatever name)
2. Assign it a number and add it to `Theme.h`:
```cpp
#define THEME_WARM 2
// ...
#elif ACTIVE_THEME == THEME_WARM
#include "themes/ThemeWarm.h"
#endif
```
3. Change `-DACTIVE_THEME=2` in `platformio.ini` and rebuild

The old theme stays in git — switch back any time by changing the flag.

---

## Token reference

| Token | Default | Meaning |
|---|---|---|
| `BG` | `0x000000` | Screen background |
| `TEXT_PRIMARY` | `0xFFFFFF` | Main text, primary labels |
| `TEXT_DIM` | `0xAAAAAA` | Secondary labels (humidity, state) |
| `TEXT_FAINT` | `0x888888` | Subtle labels, icon tints |
| `TEXT_MUTED` | `0x666666` | Arc range labels, go-back, ring indicator |
| `SURFACE` | `0x333333` | Arc tracks, off-mode color |
| `SURFACE_DARK` | `0x222222` | Inner arc track |
| `SURFACE_FAINT` | `0x444444` | Disabled arc, inactive lamp |
| `RING_ACTIVE` | `0x666666` | Timer ring indicator arc |
| `RING_BG` | `0x1E1E1E` | Timer ring background arc |
| `TEMP_COLD` | `0x0088FF` | Temp arc < 18° |
| `TEMP_MID` | `0xFFCC00` | Temp arc 18–21° |
| `TEMP_WARM` | `0xFF6600` | Temp arc ≥ 21° |
| `AC_MODE_OFF` | `0x333333` | AC off mode indicator |
| `AC_MODE_HEAT` | `0xFF4400` | AC heat mode |
| `AC_MODE_COOL` | `0x0088FF` | AC cool mode |
| `AC_MODE_AUTO` | `0x00BB44` | AC auto mode |
| `AC_MODE_FAN` | `0xBBBBBB` | AC fan mode |
| `AC_MODE_DRY` | `0xFFAA00` | AC dry mode |
| `ACCENT_RAIN` | `0x5599CC` | Rain/humidity droplet icons |
| `ACCENT_ERROR` | `0xFF3333` | Error overlay dot |
