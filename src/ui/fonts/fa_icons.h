#pragma once
#include <lvgl.h>

// FontAwesome Solid — declared in font_awesome_solid_32.c / font_awesome_solid_24.c / font_awesome_solid_18.c
extern const lv_font_t font_awesome_solid_32;
extern const lv_font_t font_awesome_solid_24;
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

// Weather condition icons
#define FA_SUN               "\xEF\x86\x85"  // U+F185 — fa-sun
#define FA_MOON              "\xEF\x86\x86"  // U+F186 — fa-moon
#define FA_CLOUD             "\xEF\x83\x82"  // U+F0C2 — fa-cloud
#define FA_CLOUD_SUN         "\xEF\x9B\x84"  // U+F6C4 — fa-cloud-sun
#define FA_CLOUD_RAIN        "\xEF\x9C\xBD"  // U+F73D — fa-cloud-rain
#define FA_CLOUD_SHOWERS     "\xEF\x9D\x80"  // U+F740 — fa-cloud-showers-heavy
#define FA_CLOUD_SUN_RAIN    "\xEF\x9D\x83"  // U+F743 — fa-cloud-sun-rain
#define FA_SMOG              "\xEF\x9D\x9F"  // U+F75F — fa-smog
#define FA_CLOUD_BOLT        "\xEF\x9D\xAC"  // U+F76C — fa-cloud-bolt
#define FA_BOLT              "\xEF\x83\xA7"  // U+F0E7 — fa-bolt

// Network / signal / status
#define FA_SIGNAL        "\xEF\x80\x92"  // U+F012 — fa-signal
#define FA_CLOCK         "\xEF\x80\x97"  // U+F017 — fa-clock
#define FA_BATTERY_HALF  "\xEF\x89\x82"  // U+F242 — fa-battery-half
#define FA_CARET_DOWN    "\xEF\x83\x97"  // U+F0D7 — fa-caret-down
#define FA_CARET_UP      "\xEF\x83\x98"  // U+F0D8 — fa-caret-up

// Indoor room icons
#define FA_BED               "\xEF\x88\xB6"  // U+F236 — fa-bed
#define FA_SHOWER            "\xEF\x8B\x8C"  // U+F2CC — fa-shower

// Music / media
#define FA_VOLUME_HIGH       "\xEF\x80\xA8"  // U+F028 — fa-volume-high
#define FA_SHUFFLE           "\xEF\x81\xB4"  // U+F074 — fa-shuffle
#define FA_REPEAT            "\xEF\x8D\xA3"  // U+F363 — fa-repeat
#define FA_MOBILE            "\xEF\x84\x8B"  // U+F10B — fa-mobile (iPhone)
#define FA_TOWER_BROADCAST   "\xEF\x94\x99"  // U+F519 — fa-tower-broadcast (Sonos)
#define FA_TV                "\xEF\x89\xAC"  // U+F26C — fa-tv (AV receiver)
