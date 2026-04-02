#include "IndoorTempsCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

// Row Y offsets from center — card area: y=-80 to y=+15, visual center y=-35
static constexpr int ROW_BALCONY  = -61;
static constexpr int ROW_BEDROOM  = -28;
static constexpr int ROW_BATHROOM =  +5;

// Within each row: icon x, temp x, droplet x, humidity x
static constexpr int COL_ICON     = -61;
static constexpr int COL_TEMP     = -15;
static constexpr int COL_DROPLET  = +30;
static constexpr int COL_HUMIDITY = +56;

static lv_obj_t* makeIcon(lv_obj_t* parent, const char* glyph) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(lbl, glyph);
    return lbl;
}

static lv_obj_t* makeTempLabel(lv_obj_t* parent) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    return lbl;
}

static lv_obj_t* makeDroplet(lv_obj_t* parent) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(Theme::ACCENT_RAIN), LV_PART_MAIN);
    lv_label_set_text(lbl, FA_DROPLET);
    return lbl;
}

static lv_obj_t* makeHumidLabel(lv_obj_t* parent) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    return lbl;
}

void IndoorTempsCard::init(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    _balconyIcon     = makeIcon(_container, FA_SUN);
    _balconyTemp     = makeTempLabel(_container);
    _balconyDroplet  = makeDroplet(_container);
    _balconyHumidity = makeHumidLabel(_container);

    _bedroomIcon     = makeIcon(_container, FA_BED);
    _bedroomTemp     = makeTempLabel(_container);
    _bedroomDroplet  = makeDroplet(_container);
    _bedroomHumidity = makeHumidLabel(_container);

    _bathroomIcon     = makeIcon(_container, FA_SHOWER);
    _bathroomTemp     = makeTempLabel(_container);
    _bathroomDroplet  = makeDroplet(_container);
    _bathroomHumidity = makeHumidLabel(_container);
}

void IndoorTempsCard::updateRow(lv_obj_t* tempLabel, lv_obj_t* humLabel, float temp, float humidity) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f°", temp);
    lv_label_set_text(tempLabel, buf);
    snprintf(buf, sizeof(buf), "%.0f%%", humidity);
    lv_label_set_text(humLabel, buf);
}

void IndoorTempsCard::update() {
    SensorState& s = appState.sensors;

    updateRow(_balconyTemp,  _balconyHumidity,  s.outdoorTemp,  s.outdoorHumidity);
    updateRow(_bedroomTemp,  _bedroomHumidity,  s.bedroomTemp,  s.bedroomHumidity);
    updateRow(_bathroomTemp, _bathroomHumidity, s.bathroomTemp, s.bathroomHumidity);

    lv_obj_align(_balconyIcon,     LV_ALIGN_CENTER, COL_ICON,     ROW_BALCONY);
    lv_obj_align(_balconyTemp,     LV_ALIGN_CENTER, COL_TEMP,     ROW_BALCONY);
    lv_obj_align(_balconyDroplet,  LV_ALIGN_CENTER, COL_DROPLET,  ROW_BALCONY);
    lv_obj_align(_balconyHumidity, LV_ALIGN_CENTER, COL_HUMIDITY, ROW_BALCONY);

    lv_obj_align(_bedroomIcon,     LV_ALIGN_CENTER, COL_ICON,     ROW_BEDROOM);
    lv_obj_align(_bedroomTemp,     LV_ALIGN_CENTER, COL_TEMP,     ROW_BEDROOM);
    lv_obj_align(_bedroomDroplet,  LV_ALIGN_CENTER, COL_DROPLET,  ROW_BEDROOM);
    lv_obj_align(_bedroomHumidity, LV_ALIGN_CENTER, COL_HUMIDITY, ROW_BEDROOM);

    lv_obj_align(_bathroomIcon,     LV_ALIGN_CENTER, COL_ICON,     ROW_BATHROOM);
    lv_obj_align(_bathroomTemp,     LV_ALIGN_CENTER, COL_TEMP,     ROW_BATHROOM);
    lv_obj_align(_bathroomDroplet,  LV_ALIGN_CENTER, COL_DROPLET,  ROW_BATHROOM);
    lv_obj_align(_bathroomHumidity, LV_ALIGN_CENTER, COL_HUMIDITY, ROW_BATHROOM);
}

void IndoorTempsCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void IndoorTempsCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
