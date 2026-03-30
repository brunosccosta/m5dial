# Dev Guide

Project-specific setup and tooling instructions.

---

## Regenerating icon fonts

Icon fonts live in `src/ui/fonts/`. The current font (`font_awesome_solid_32.c`) is committed — only regenerate when adding new glyphs or changing size/bpp.

### Step 1 — get the TTF

FontAwesome Free ships as WOFF2. Download from [fontawesome.com](https://fontawesome.com) (Free tier, solid variant).

Convert to TTF with Python:
```bash
pip install fonttools brotli
python3
>>> from fontTools.ttLib.woff2 import decompress
>>> decompress('fa-solid-900.woff2', 'fa-solid-900.ttf')
```

### Step 2 — generate the LVGL font file

Install the converter:
```bash
npm i lv_font_conv -g
```

Run with the current codepoints (decimal, comma-separated):
```bash
lv_font_conv --bpp 4 --size 32 --no-compress \
  --font fa-solid-900.ttf \
  --range 61675,62172,61549,61459,61473,63587,61507,61457,63278 \
  --format lvgl -o font_awesome_solid_32.c
```

Move the output to `src/ui/fonts/`.

### Current codepoints

| Decimal | Hex | FA name | Macro |
|---|---|---|---|
| 61675 | F0EB | fa-lightbulb | `FA_LIGHTBULB` |
| 62172 | F2DC | fa-snowflake | `FA_SNOWFLAKE` |
| 61549 | F06D | fa-fire | `FA_FIRE` |
| 61459 | F013 | fa-gear | `FA_GEAR` |
| 61473 | F021 | fa-arrows-rotate | `FA_ARROWS_ROTATE` |
| 63587 | F863 | fa-fan | `FA_FAN` |
| 61507 | F043 | fa-droplet | `FA_DROPLET` |
| 61457 | F011 | fa-power-off | `FA_POWER_OFF` |
| 63278 | F72E | fa-wind | `FA_WIND` |

### Gotchas

- Do **not** use `--lcd` — it enables subpixel rendering which causes glyphs to render as a partial sliver on the GC9A01 display
- `--bpp 4` gives antialiased edges; `--bpp 1` is sharp but aliased — 4 looks better on this display
- When adding a new glyph, also add its `#define` to `src/ui/fonts/fa_icons.h` and update the codepoint table above
