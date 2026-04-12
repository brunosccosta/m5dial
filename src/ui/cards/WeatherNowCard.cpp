#include "WeatherNowCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"
#include "../CardLayout.h"

// Condition map — key matches sensor.detailed_condition state values.
// To add/change an icon: update the icon field below; only this table needs touching.
struct ConditionInfo {
    const char* key;
    const char* label;
    const char* icon;
};

static const ConditionInfo CONDITIONS[] = {
    { "clear",                    "Clear",           nullptr           },  // day/night resolved in update()
    { "partlycloudy",             "Partly Cloudy",   FA_CLOUD_SUN      },
    { "partlycloudy-fog",         "Cloudy & Foggy",  FA_SMOG           },
    { "partlycloudy-light-rain",  "Cloudy & Drizzle",FA_CLOUD_SUN_RAIN },
    { "partlycloudy-rain",        "Cloudy & Rain",   FA_CLOUD_SUN_RAIN },
    { "cloudy",                   "Cloudy",          FA_CLOUD          },
    { "fog",                      "Foggy",           FA_SMOG           },
    { "rainy",                    "Rainy",           FA_CLOUD_SHOWERS  },
    { "light-rain",               "Light Rain",      FA_CLOUD_RAIN     },
    { "light-snow",               "Light Snow",      FA_SNOWFLAKE      },
    { "partlycloudy-light-snow",  "Cloudy & Sleet",  FA_CLOUD_RAIN     },
    { "partlycloudy-snow",        "Cloudy & Snow",   FA_SNOWFLAKE      },
    { "partlycloudy-lightning",   "Cloudy & Storm",  FA_CLOUD_BOLT     },
    { "snowy",                    "Snowy",           FA_SNOWFLAKE      },
    { "snowy-rainy",              "Sleet",           FA_CLOUD_RAIN     },
    { "lightning",                "Thunderstorm",    FA_BOLT           },
};
static constexpr int CONDITION_COUNT = sizeof(CONDITIONS) / sizeof(CONDITIONS[0]);

static const ConditionInfo* conditionInfo(const char* key) {
    for (int i = 0; i < CONDITION_COUNT; i++)
        if (strcmp(CONDITIONS[i].key, key) == 0) return &CONDITIONS[i];
    return nullptr;
}

// --- Lifecycle ---

void WeatherNowCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent);

    // Row 1: temperature (hero) + feels-like
    lv_obj_t* tempRow = CardLayout::makeRow(_container);
    _tempLabel = lv_label_create(tempRow);
    lv_obj_set_style_text_font(_tempLabel, CardLayout::fontHero(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_tempLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_tempLabel, "--°");
    _feelsLikeLabel = lv_label_create(tempRow);
    lv_obj_set_style_text_font(_feelsLikeLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_feelsLikeLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_feelsLikeLabel, "");

    // Row 2: condition icon + condition text
    lv_obj_t* condRow = CardLayout::makeRow(_container);
    _iconLabel = lv_label_create(condRow);
    lv_obj_set_style_text_font(_iconLabel, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_iconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_iconLabel, "");
    _conditionLabel = lv_label_create(condRow);
    lv_obj_set_style_text_font(_conditionLabel, CardLayout::fontValue(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_conditionLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_conditionLabel, "---");
}

void WeatherNowCard::update() {
    WeatherState& w = appState.weather;

    if (w.valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f°", w.temperature);
        lv_label_set_text(_tempLabel, buf);
        snprintf(buf, sizeof(buf), "/ %.0f°", w.feelsLike);
        lv_label_set_text(_feelsLikeLabel, buf);

        const ConditionInfo* info = (w.detailedCondition[0] != '\0')
            ? conditionInfo(w.detailedCondition)
            : nullptr;
        const char* icon = nullptr;
        if (info) {
            icon = (info->icon == nullptr)
                ? (w.isDaytime ? FA_SUN : FA_MOON)
                : info->icon;
        }
        lv_label_set_text(_iconLabel,      icon ? icon : "");
        lv_label_set_text(_conditionLabel, info ? info->label : w.condition);
    } else {
        lv_label_set_text(_tempLabel,      "--°");
        lv_label_set_text(_feelsLikeLabel, "");
        lv_label_set_text(_iconLabel,      "");
        lv_label_set_text(_conditionLabel, "---");
    }

    // If condition + icon together exceed the usable width, cap and scroll.
    static constexpr int MAX_TOTAL = 200;
    lv_label_set_long_mode(_conditionLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_conditionLabel, LV_SIZE_CONTENT);
    lv_obj_update_layout(_container);

    bool hasIcon = lv_label_get_text(_iconLabel)[0] != '\0';
    int  iconW   = hasIcon ? lv_obj_get_width(_iconLabel) + CardLayout::PAD_ICON_TEXT : 0;
    int  textW   = lv_obj_get_width(_conditionLabel);

    if (iconW + textW > MAX_TOTAL) {
        lv_obj_set_width(_conditionLabel, MAX_TOTAL - iconW);
        lv_label_set_long_mode(_conditionLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_anim_duration(_conditionLabel, 14000, LV_PART_MAIN);
    }
}

void WeatherNowCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); }
void WeatherNowCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
