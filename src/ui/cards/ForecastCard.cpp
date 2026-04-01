#include "ForecastCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

// Condition → icon mapping (same keys as WeatherNowCard; "clear" always → FA_SUN for forecasts)
struct FcConditionInfo { const char* key; const char* icon; };
static const FcConditionInfo FC_CONDITIONS[] = {
    { "clear",                    FA_SUN            },
    { "partlycloudy",             FA_CLOUD_SUN      },
    { "partlycloudy-fog",         FA_SMOG           },
    { "partlycloudy-light-rain",  FA_CLOUD_SUN_RAIN },
    { "partlycloudy-rain",        FA_CLOUD_SUN_RAIN },
    { "cloudy",                   FA_CLOUD          },
    { "fog",                      FA_SMOG           },
    { "rainy",                    FA_CLOUD_SHOWERS  },
    { "light-rain",               FA_CLOUD_RAIN     },
    { "light-snow",               FA_SNOWFLAKE      },
    { "partlycloudy-light-snow",  FA_CLOUD_RAIN     },
    { "partlycloudy-snow",        FA_SNOWFLAKE      },
    { "partlycloudy-lightning",   FA_CLOUD_BOLT     },
    { "snowy",                    FA_SNOWFLAKE      },
    { "snowy-rainy",              FA_CLOUD_RAIN     },
    { "lightning",                FA_BOLT           },
};
static constexpr int FC_CONDITION_COUNT = sizeof(FC_CONDITIONS) / sizeof(FC_CONDITIONS[0]);

static const char* fcIcon(const char* key) {
    for (int i = 0; i < FC_CONDITION_COUNT; i++)
        if (strcmp(FC_CONDITIONS[i].key, key) == 0) return FC_CONDITIONS[i].icon;
    return nullptr;
}

// Column x positions (left / right of center)
static constexpr int COL_X[2] = { -50, +50 };

// --- Lifecycle ---

void ForecastCard::init(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    // --- Left column (today) ---
    _headerLeft = lv_label_create(_container);
    lv_obj_set_style_text_font(_headerLeft, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_headerLeft, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_obj_align(_headerLeft, LV_ALIGN_CENTER, COL_X[0], -75);

    _iconLeft = lv_label_create(_container);
    lv_obj_set_style_text_font(_iconLeft, &font_awesome_solid_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_iconLeft, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_iconLeft, LV_ALIGN_CENTER, COL_X[0], -51);

    _tempLeft = lv_label_create(_container);
    lv_obj_set_style_text_font(_tempLeft, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tempLeft, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_tempLeft, LV_ALIGN_CENTER, COL_X[0] + 3, -24);

    _rainIconLeft = lv_label_create(_container);
    lv_obj_set_style_text_font(_rainIconLeft, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_rainIconLeft, lv_color_hex(Theme::ACCENT_RAIN), LV_PART_MAIN);
    lv_obj_align(_rainIconLeft, LV_ALIGN_CENTER, COL_X[0] - 12, -5);

    _rainLeft = lv_label_create(_container);
    lv_obj_set_style_text_font(_rainLeft, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_rainLeft, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_align(_rainLeft, LV_ALIGN_CENTER, COL_X[0] + 12, -5);

    // --- Right column (tomorrow) ---
    _headerRight = lv_label_create(_container);
    lv_obj_set_style_text_font(_headerRight, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_headerRight, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_obj_align(_headerRight, LV_ALIGN_CENTER, COL_X[1], -75);

    _iconRight = lv_label_create(_container);
    lv_obj_set_style_text_font(_iconRight, &font_awesome_solid_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_iconRight, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_iconRight, LV_ALIGN_CENTER, COL_X[1], -51);

    _tempRight = lv_label_create(_container);
    lv_obj_set_style_text_font(_tempRight, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tempRight, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_tempRight, LV_ALIGN_CENTER, COL_X[1] + 3, -24);

    _rainIconRight = lv_label_create(_container);
    lv_obj_set_style_text_font(_rainIconRight, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_rainIconRight, lv_color_hex(Theme::ACCENT_RAIN), LV_PART_MAIN);
    lv_obj_align(_rainIconRight, LV_ALIGN_CENTER, COL_X[1] - 12, -5);

    _rainRight = lv_label_create(_container);
    lv_obj_set_style_text_font(_rainRight, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_rainRight, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_align(_rainRight, LV_ALIGN_CENTER, COL_X[1] + 10, -5);

    // Static content never changes
    lv_label_set_text(_headerLeft,    "TODAY");
    lv_label_set_text(_headerRight,   "TMR");
    lv_label_set_text(_rainIconLeft,  FA_DROPLET);
    lv_label_set_text(_rainIconRight, FA_DROPLET);
}

void ForecastCard::update() {
    for (int col = 0; col < 2; col++) updateColumn(col);

    lv_obj_align(_headerLeft,   LV_ALIGN_CENTER, COL_X[0], -75);
    lv_obj_align(_iconLeft,     LV_ALIGN_CENTER, COL_X[0], -51);
    lv_obj_align(_tempLeft,     LV_ALIGN_CENTER, COL_X[0] + 3, -24);
    lv_obj_align(_rainIconLeft, LV_ALIGN_CENTER, COL_X[0] - 13, +3);
    lv_obj_align(_rainLeft,     LV_ALIGN_CENTER, COL_X[0] + 11, +3);

    lv_obj_align(_headerRight,   LV_ALIGN_CENTER, COL_X[1], -75);
    lv_obj_align(_iconRight,     LV_ALIGN_CENTER, COL_X[1], -51);
    lv_obj_align(_tempRight,     LV_ALIGN_CENTER, COL_X[1] + 3, -24);
    lv_obj_align(_rainIconRight, LV_ALIGN_CENTER, COL_X[1] - 13, +3);
    lv_obj_align(_rainRight,     LV_ALIGN_CENTER, COL_X[1] + 11, +3);
}

void ForecastCard::updateColumn(int col) {
    ForecastDay& f = (col == 0) ? appState.forecastToday : appState.forecastTomorrow;

    lv_obj_t* iconLbl  = (col == 0) ? _iconLeft  : _iconRight;
    lv_obj_t* tempLbl  = (col == 0) ? _tempLeft  : _tempRight;
    lv_obj_t* rainLbl  = (col == 0) ? _rainLeft  : _rainRight;

    if (f.valid) {
        const char* icon = fcIcon(f.detailedCondition);
        lv_label_set_text(iconLbl, icon ? icon : "");

        char buf[12];
        snprintf(buf, sizeof(buf), "%.0f°", f.temperature);
        lv_label_set_text(tempLbl, buf);

        snprintf(buf, sizeof(buf), "%d%%", f.rainChance);
        lv_label_set_text(rainLbl, buf);
    } else {
        lv_label_set_text(iconLbl, "");
        lv_label_set_text(tempLbl, "--°");
        lv_label_set_text(rainLbl, "--%");
    }
}

void ForecastCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); }
void ForecastCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
