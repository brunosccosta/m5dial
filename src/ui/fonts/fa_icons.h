#pragma once
#include <lvgl.h>

// FontAwesome Solid — declared in font_awesome_solid_32.c / font_awesome_solid_18.c
extern const lv_font_t font_awesome_solid_32;
extern const lv_font_t font_awesome_solid_18;

// UTF-8 encoded glyph strings (U+F000 range → 3-byte sequences)
#define FA_LIGHTBULB      "\xEF\x83\xAB"  // U+F0EB — fa-lightbulb
#define FA_SNOWFLAKE      "\xEF\x8B\x9C"  // U+F2DC — fa-snowflake
#define FA_FIRE           "\xEF\x81\xAD"  // U+F06D — fa-fire
#define FA_GEAR           "\xEF\x80\x93"  // U+F013 — fa-gear
#define FA_ARROWS_ROTATE  "\xEF\x80\xA1"  // U+F021 — fa-arrows-rotate
#define FA_FAN            "\xEF\xA1\xA3"  // U+F863 — fa-fan
#define FA_DROPLET        "\xEF\x81\x83"  // U+F043 — fa-droplet
#define FA_POWER_OFF      "\xEF\x80\x91"  // U+F011 — fa-power-off
#define FA_WIND           "\xEF\x9C\xAE"  // U+F72E — fa-wind
