#include "SpotifyCard.h"
#include "../../AppState.h"
#include "../CardLayout.h"
#include "../fonts/fa_icons.h"
#include "../Theme.h"

static const char* sourceIcon(const char* source) {
    if (strcmp(source, "iPhone")       == 0) return FA_MOBILE;
    if (strcmp(source, "Sala")         == 0) return FA_TOWER_BROADCAST;
    if (strcmp(source, "Living Room")  == 0) return FA_TV;
    return FA_TOWER_BROADCAST;
}

void SpotifyCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent, 0, CardLayout::PAD_ROW);

    // State — "playing" / "paused"
    _stateLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_stateLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_stateLabel, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_label_set_text(_stateLabel, "—");

    // Title — scrolls if too long
    _titleLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_titleLabel, CardLayout::fontValue(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_titleLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(_titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(_titleLabel, 180);
    lv_label_set_long_mode(_titleLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(_titleLabel, 19000, LV_PART_MAIN);
    lv_label_set_text(_titleLabel, "—");

    // Artist — scrolls if too long
    _artistLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_artistLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_artistLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_align(_artistLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(_artistLabel, 180);
    lv_label_set_long_mode(_artistLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(_artistLabel, 14000, LV_PART_MAIN);
    lv_label_set_text(_artistLabel, "");

    // Details row: source (left) ←→ volume (right), space-between
    lv_obj_t* detailsRow = lv_obj_create(_container);
    lv_obj_set_size(detailsRow, 180, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(detailsRow, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(detailsRow, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(detailsRow, 0, LV_PART_MAIN);
    lv_obj_set_layout(detailsRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(detailsRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(detailsRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(detailsRow, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* srcGroup = CardLayout::makeRow(detailsRow, CardLayout::PAD_ICON_TEXT);
    _srcIconLabel = lv_label_create(srcGroup);
    lv_obj_set_style_text_font(_srcIconLabel, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_srcIconLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_srcIconLabel, FA_MOBILE);

    _srcLabel = lv_label_create(srcGroup);
    lv_obj_set_style_text_font(_srcLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_srcLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_obj_set_width(_srcLabel, 60);
    lv_label_set_long_mode(_srcLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(_srcLabel, 10000, LV_PART_MAIN);
    lv_label_set_text(_srcLabel, "");

    lv_obj_t* volGroup = CardLayout::makeRow(detailsRow, CardLayout::PAD_ICON_TEXT);
    lv_obj_t* volIcon = lv_label_create(volGroup);
    lv_obj_set_style_text_font(volIcon, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(volIcon, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(volIcon, FA_VOLUME_HIGH);

    _volLabel = lv_label_create(volGroup);
    lv_obj_set_style_text_font(_volLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_volLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    lv_label_set_text(_volLabel, "");

    // Toggles row: shuffle + repeat always visible; color indicates active state
    lv_obj_t* togglesRow = CardLayout::makeRow(_container, 20);

    _shuffleLabel = lv_label_create(togglesRow);
    lv_obj_set_style_text_font(_shuffleLabel, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_shuffleLabel, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_label_set_text(_shuffleLabel, FA_SHUFFLE);

    _repeatLabel = lv_label_create(togglesRow);
    lv_obj_set_style_text_font(_repeatLabel, CardLayout::fontIcon(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_repeatLabel, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_label_set_text(_repeatLabel, FA_REPEAT);
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
        lv_label_set_text(_volLabel,    "");
        lv_label_set_text(_srcLabel,    "");
        lv_obj_set_style_text_color(_shuffleLabel, lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
        lv_obj_set_style_text_color(_repeatLabel,  lv_color_hex(Theme::TEXT_MUTED), LV_PART_MAIN);
        return;
    }

    lv_label_set_text(_stateLabel,  s.state);
    lv_label_set_text(_titleLabel,  s.title[0]  ? s.title  : "—");
    lv_label_set_text(_artistLabel, s.artist[0] ? s.artist : "");

    lv_obj_set_style_text_color(_shuffleLabel,
        lv_color_hex(s.shuffle ? Theme::AC_MODE_AUTO : Theme::TEXT_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_color(_repeatLabel,
        lv_color_hex(strcmp(s.repeat, "off") != 0 ? Theme::AC_MODE_AUTO : Theme::TEXT_MUTED), LV_PART_MAIN);

    char buf[8];
    snprintf(buf, sizeof(buf), "%.0f%%", s.volume * 100);
    lv_label_set_text(_volLabel, buf);

    lv_label_set_text(_srcIconLabel, sourceIcon(s.source));
    lv_label_set_text(_srcLabel,     s.source[0] ? s.source : "");
}

bool SpotifyCard::isVisible() const {
    const SpotifyState& s = appState.spotify;
    return s.valid
        && (strcmp(s.state, "playing") == 0 || strcmp(s.state, "paused") == 0);
}

void SpotifyCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); }
void SpotifyCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
