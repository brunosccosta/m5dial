#include "WeatherNowCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

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
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    _tempLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_tempLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(_tempLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_tempLabel, LV_ALIGN_CENTER, -9, -45);

    _feelsLikeLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_feelsLikeLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_feelsLikeLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_feelsLikeLabel, LV_ALIGN_CENTER, 20, -45);

    _iconLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_iconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_iconLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(_iconLabel, LV_ALIGN_CENTER, -40, -8);

    _conditionLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_conditionLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_conditionLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_conditionLabel, LV_ALIGN_CENTER, 0, -8);
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
                ? (appState.weather.isDaytime ? FA_SUN : FA_MOON)  // "clear" — day/night
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
    lv_obj_align(_tempLabel, LV_ALIGN_CENTER, -9, -45);
    lv_obj_update_layout(_container);
    lv_obj_align_to(_feelsLikeLabel, _tempLabel, LV_ALIGN_OUT_RIGHT_BOTTOM, 6, -2);
    lv_obj_align(_iconLabel, LV_ALIGN_CENTER, -40, -8);
    lv_obj_update_layout(_container);
    lv_obj_align_to(_conditionLabel, _iconLabel, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
}

void WeatherNowCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); }
void WeatherNowCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
