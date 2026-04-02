# TODO

Small improvements and known issues parked for later.

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

## Buzzer feedback

M5Dial has an 80dB buzzer via `M5Dial.Speaker`. Potential uses: confirmation beep on button press, alert on sensor threshold (e.g. room too hot), error sound on WiFi/HA disconnect, encoder tick tone. Investigate what feels useful vs. annoying.

---

## Screen transition animations

Push/pop transitions (rest → menu, menu → control) are still bare `lv_scr_load()` calls. LVGL has `lv_scr_load_anim()` — could add a fade or slide. MenuScreen already has internal slide animation between cards; the same approach could extend to screen-level transitions.

---
