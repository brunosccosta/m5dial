#!/usr/bin/env bash
# Regenerate all FontAwesome Solid LVGL font files.
# To add a glyph: append its decimal codepoint to GLYPHS, then add a #define to src/ui/fonts/fa_icons.h.
# Run from the repo root.

set -e

FONT="tools/fonts/fa-solid-900.ttf"
OUT="src/ui/fonts"

GLYPHS="61457,61458,61459,61463,61473,61480,61507,61549,61556,61634,61655,61656,61671,61675,61707,61829,61830,62006,62018,62060,62156,62172,62307,62745,63172,63278,63293,63296,63299,63327,63340,63587"

lv_font_conv --bpp 4 --size 32 --no-compress --font "$FONT" --range "$GLYPHS" --format lvgl -o "$OUT/font_awesome_solid_32.c"
lv_font_conv --bpp 4 --size 24 --no-compress --font "$FONT" --range "$GLYPHS" --format lvgl -o "$OUT/font_awesome_solid_24.c"
lv_font_conv --bpp 4 --size 18 --no-compress --font "$FONT" --range "$GLYPHS" --format lvgl -o "$OUT/font_awesome_solid_18.c"

echo "Done."
