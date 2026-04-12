#include "ForecastCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"
#include "../CardLayout.h"

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

static constexpr int COL_GAP = 30; // gap between the two day columns

static const char* fcIcon(const char* key) {
    for (int i = 0; i < FC_CONDITION_COUNT; i++)
        if (strcmp(FC_CONDITIONS[i].key, key) == 0) return FC_CONDITIONS[i].icon;
    return nullptr;
}

static lv_obj_t* makeDay(lv_obj_t* parent, const char* header,
                         lv_obj_t** iconOut, lv_obj_t** tempOut, lv_obj_t** rainOut) {
    lv_obj_t* col = CardLayout::makeColumn(parent, CardLayout::PAD_ROW_LIST);

    lv_obj_t* hdr = lv_label_create(col);
    lv_obj_set_style_text_font(hdr, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(hdr, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_label_set_text(hdr, header);

    *iconOut = lv_label_create(col);
    lv_obj_set_style_text_font(*iconOut, &font_awesome_solid_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(*iconOut, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(*iconOut, "");

    *tempOut = lv_label_create(col);
    lv_obj_set_style_text_font(*tempOut, CardLayout::fontTitle(), LV_PART_MAIN);
    lv_obj_set_style_text_color(*tempOut, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(*tempOut, "--°");

    lv_obj_t* rainRow = CardLayout::makeRow(col, CardLayout::PAD_ICON_TEXT);
    lv_obj_t* rainIcon = lv_label_create(rainRow);
    lv_obj_set_style_text_font(rainIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(rainIcon, lv_color_hex(Theme::ACCENT_RAIN), LV_PART_MAIN);
    lv_label_set_text(rainIcon, FA_CLOUD_RAIN);

    *rainOut = lv_label_create(rainRow);
    lv_obj_set_style_text_font(*rainOut, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(*rainOut, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(*rainOut, "--%");

    return col;
}

// --- Lifecycle ---

void ForecastCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent);

    lv_obj_t* cols = CardLayout::makeRow(_container, COL_GAP);
    makeDay(cols, "TODAY", &_iconLeft,  &_tempLeft,  &_rainLeft);
    makeDay(cols, "TMR",   &_iconRight, &_tempRight, &_rainRight);
}

void ForecastCard::update() {
    updateColumn(0);
    updateColumn(1);
}

void ForecastCard::updateColumn(int col) {
    ForecastDay& f = (col == 0) ? appState.forecastToday : appState.forecastTomorrow;

    lv_obj_t* iconLbl = (col == 0) ? _iconLeft  : _iconRight;
    lv_obj_t* tempLbl = (col == 0) ? _tempLeft  : _tempRight;
    lv_obj_t* rainLbl = (col == 0) ? _rainLeft  : _rainRight;

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

void ForecastCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void ForecastCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
