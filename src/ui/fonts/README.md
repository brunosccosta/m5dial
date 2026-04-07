# Font Icons

Two FontAwesome Solid bitmap fonts compiled into LVGL format. Both contain the same glyphs at different sizes:

| File | Size | Used by |
|---|---|---|
| `font_awesome_solid_32.c` | 32px | CarouselMenu, RestScreen device strip |
| `font_awesome_solid_24.c` | 24px | ForecastCard condition icons |
| `font_awesome_solid_18.c` | 18px | ACControlScreen mode indicator, ForecastCard rain icon |

Glyph macros and font extern declarations live in `fa_icons.h` — the only file you need to include.

---

## How to add a new icon

### Step 1 — find the codepoint

Look up the icon on [fontawesome.com](https://fontawesome.com) (Free, solid variant). Note the Unicode codepoint (e.g. `U+F185` for fa-sun).

Convert to decimal for the `--range` flag: `0xF185` = `61829`.

### Step 2 — get the TTF

FontAwesome Free ships as WOFF2. Convert to TTF with Python:

```bash
pip install fonttools brotli
python3
>>> from fontTools.ttLib.woff2 import decompress
>>> decompress('fa-solid-900.woff2', 'fa-solid-900.ttf')
```

### Step 3 — regenerate all font files

Add the new decimal codepoint to `GLYPHS` in `tools/gen_fonts.sh`, then run from the repo root:

```bash
bash tools/gen_fonts.sh
```

This regenerates all three sizes (32px, 24px, 18px) in one shot from the single canonical glyph list.

### Step 4 — declare the macro

Add the UTF-8 encoded string to `fa_icons.h`:

```cpp
// U+F185 encodes as: 0xEF 0x86 0x85
#define FA_SUN  "\xEF\x86\x85"  // U+F185 — fa-sun
```

To compute UTF-8 bytes for any U+F... codepoint:
- Byte 1: `0xEF`
- Byte 2: `0x80 | ((cp >> 6) & 0x3F)`
- Byte 3: `0x80 | (cp & 0x3F)`

Update the codepoints table below.

---

## Current glyphs

| Decimal | Hex | FA name | Macro |
|---|---|---|---|
| 61457 | F011 | fa-power-off | `FA_POWER_OFF` |
| 61461 | F015 | fa-house | `FA_HOUSE` |
| 61459 | F013 | fa-gear | `FA_GEAR` |
| 61473 | F021 | fa-arrows-rotate | `FA_ARROWS_ROTATE` |
| 61507 | F043 | fa-droplet | `FA_DROPLET` |
| 61549 | F06D | fa-fire | `FA_FIRE` |
| 61671 | F0E7 | fa-bolt | `FA_BOLT` |
| 61675 | F0EB | fa-lightbulb | `FA_LIGHTBULB` |
| 61634 | F0C2 | fa-cloud | `FA_CLOUD` |
| 61829 | F185 | fa-sun | `FA_SUN` |
| 61931 | F1EB | fa-wifi | `FA_WIFI` |
| 61830 | F186 | fa-moon | `FA_MOON` |
| 62172 | F2DC | fa-snowflake | `FA_SNOWFLAKE` |
| 63172 | F6C4 | fa-cloud-sun | `FA_CLOUD_SUN` |
| 63278 | F72E | fa-wind | `FA_WIND` |
| 63293 | F73D | fa-cloud-rain | `FA_CLOUD_RAIN` |
| 63296 | F740 | fa-cloud-showers-heavy | `FA_CLOUD_SHOWERS` |
| 63299 | F743 | fa-cloud-sun-rain | `FA_CLOUD_SUN_RAIN` |
| 63327 | F75F | fa-smog | `FA_SMOG` |
| 63340 | F76C | fa-cloud-bolt | `FA_CLOUD_BOLT` |
| 62006 | F236 | fa-bed | `FA_BED` |
| 62156 | F2CC | fa-shower | `FA_SHOWER` |
| 63587 | F863 | fa-fan | `FA_FAN` |
| 61480 | F028 | fa-volume-high | `FA_VOLUME_HIGH` |
| 61556 | F074 | fa-shuffle | `FA_SHUFFLE` |
| 62307 | F363 | fa-repeat | `FA_REPEAT` |
| 61707 | F10B | fa-mobile | `FA_MOBILE` |
| 62745 | F519 | fa-tower-broadcast | `FA_TOWER_BROADCAST` |
| 62060 | F26C | fa-tv | `FA_TV` |

---

## Using icons in code

```cpp
#include "../fonts/fa_icons.h"

// Icon glyph — FA font
lv_obj_set_style_text_font(_iconLabel, &font_awesome_solid_18, LV_PART_MAIN);
lv_label_set_text(_iconLabel, FA_FIRE);

// Text label — Montserrat font (separate object — LVGL can't mix fonts in one label)
lv_obj_set_style_text_font(_textLabel, &lv_font_montserrat_14, LV_PART_MAIN);
lv_label_set_text(_textLabel, "heat");
```

To place text immediately after the icon, align the text relative to the icon after layout is resolved:

```cpp
lv_obj_align(_iconLabel, LV_ALIGN_CENTER, -20, y);
lv_obj_update_layout(screen);  // must call before align_to — icon width must be known
lv_obj_align_to(_textLabel, _iconLabel, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
```

---

## Gotchas

- **No `--lcd`** — subpixel rendering causes glyphs to render as a partial sliver on the GC9A01 display
- **`--bpp 4`** — antialiased edges; `--bpp 1` is aliased — 4 looks better on this screen
- **Regenerate both sizes** — 32px and 18px must always have the same glyph set or you'll get missing-glyph boxes at one size
