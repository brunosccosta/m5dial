#include "SpotifyCard.h"
#include "../../AppState.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

static const char* sourceIcon(const char* source) {
    if (strcmp(source, "iPhone")       == 0) return FA_MOBILE;
    if (strcmp(source, "Sala")         == 0) return FA_TOWER_BROADCAST;
    if (strcmp(source, "Living Room")  == 0) return FA_TV;
    return FA_TOWER_BROADCAST; // fallback
}

void SpotifyCard::init(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 240, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_CLICKABLE);

    // State — "playing" / "paused" / "idle"
    _stateLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_stateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_stateLabel, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_obj_align(_stateLabel, LV_ALIGN_CENTER, 0, -77);
    lv_label_set_text(_stateLabel, "—");

    // Title — scrolls if too long
    _titleLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_titleLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(_titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(_titleLabel, 180);
    lv_label_set_long_mode(_titleLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(_titleLabel, 19000, LV_PART_MAIN);
    lv_obj_align(_titleLabel, LV_ALIGN_CENTER, 0, -40);
    lv_label_set_text(_titleLabel, "—");

    // Artist — scrolls if too long
    _artistLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_artistLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_artistLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_align(_artistLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(_artistLabel, 180);
    lv_label_set_long_mode(_artistLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(_artistLabel, 14000, LV_PART_MAIN);
    lv_obj_align(_artistLabel, LV_ALIGN_CENTER, 0, -12);
    lv_label_set_text(_artistLabel, "");

    // Shuffle icon — hidden unless active
    _shuffleLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_shuffleLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_shuffleLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_shuffleLabel, LV_ALIGN_CENTER, -12, +36);
    lv_label_set_text(_shuffleLabel, FA_SHUFFLE);
    lv_obj_add_flag(_shuffleLabel, LV_OBJ_FLAG_HIDDEN);

    // Repeat icon — hidden unless active
    _repeatLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_repeatLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_repeatLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_repeatLabel, LV_ALIGN_CENTER, +12, +36);
    lv_label_set_text(_repeatLabel, FA_REPEAT);
    lv_obj_add_flag(_repeatLabel, LV_OBJ_FLAG_HIDDEN);

    // Volume — icon + text (left side)
    _volIconLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_volIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_volIconLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_volIconLabel, LV_ALIGN_CENTER, +40, +14);
    lv_label_set_text(_volIconLabel, FA_VOLUME_HIGH);

    _volLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_volLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_volLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_volLabel, LV_ALIGN_CENTER, +76, +14);
    lv_label_set_text(_volLabel, "");

    // Source — icon + text (right side)
    _srcIconLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_srcIconLabel, &font_awesome_solid_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(_srcIconLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_align(_srcIconLabel, LV_ALIGN_CENTER, -82, +14);
    lv_label_set_text(_srcIconLabel, FA_MOBILE);

    _srcLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_srcLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(_srcLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_set_width(_srcLabel, 80);
    lv_obj_set_style_text_align(_srcLabel, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_long_mode(_srcLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(_srcLabel, 10000, LV_PART_MAIN);
    lv_obj_align(_srcLabel, LV_ALIGN_CENTER, -29, +14);
    lv_label_set_text(_srcLabel, "");
}

void SpotifyCard::update() {
    SpotifyState& s = appState.spotify;

    bool idle = !s.valid
             || (strcmp(s.state, "playing") != 0
             &&  strcmp(s.state, "paused")  != 0);

    if (idle) {
        lv_label_set_text(_stateLabel,  "—");
        lv_label_set_text(_titleLabel,  "—");
        lv_label_set_text(_artistLabel, "");
        lv_obj_add_flag(_shuffleLabel,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_repeatLabel,   LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(_volLabel,    "");
        lv_label_set_text(_srcLabel,    "");
        return;
    }

    lv_label_set_text(_stateLabel,  s.state);
    lv_label_set_text(_titleLabel,  s.title[0]  ? s.title  : "—");
    lv_label_set_text(_artistLabel, s.artist[0] ? s.artist : "");

    // Shuffle / repeat icons
    if (s.shuffle) lv_obj_clear_flag(_shuffleLabel, LV_OBJ_FLAG_HIDDEN);
    else           lv_obj_add_flag(_shuffleLabel,   LV_OBJ_FLAG_HIDDEN);

    if (strcmp(s.repeat, "off") != 0) lv_obj_clear_flag(_repeatLabel, LV_OBJ_FLAG_HIDDEN);
    else                               lv_obj_add_flag(_repeatLabel,   LV_OBJ_FLAG_HIDDEN);

    // Volume
    char buf[8];
    snprintf(buf, sizeof(buf), "%.0f%%", s.volume * 100);
    lv_label_set_text(_volLabel, buf);

    // Source
    lv_label_set_text(_srcIconLabel, sourceIcon(s.source));
    lv_label_set_text(_srcLabel,     s.source[0] ? s.source : "");
}

void SpotifyCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); }
void SpotifyCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
