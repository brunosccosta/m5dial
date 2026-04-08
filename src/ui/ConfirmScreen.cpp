#include "ConfirmScreen.h"
#include "Theme.h"

ConfirmScreen confirmScreen;

void ConfirmScreen::setup(const char* question,
                           std::function<void()> onYes,
                           std::function<void()> onNo) {
    _question = question;
    _onYes    = onYes;
    _onNo     = onNo;
}

// --- Lifecycle ---

void ConfirmScreen::init() {
    if (_initialized) return;
    _initialized = true;

    _lvScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_lvScreen, lv_color_hex(Theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_lvScreen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(_lvScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Question
    _questionLabel = lv_label_create(_lvScreen);
    lv_obj_set_style_text_font(_questionLabel, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(_questionLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_questionLabel, "");
    lv_obj_align(_questionLabel, LV_ALIGN_CENTER, 0, -30);

    // Yes container (clickable)
    _yesContainer = lv_obj_create(_lvScreen);
    lv_obj_set_size(_yesContainer, 80, 56);
    lv_obj_set_style_bg_opa(_yesContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_yesContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_yesContainer, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_yesContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_yesContainer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(_yesContainer, LV_ALIGN_CENTER, -50, 22);
    lv_obj_add_event_cb(_yesContainer, onYesClick, LV_EVENT_CLICKED, this);

    _yesIconLabel = lv_label_create(_yesContainer);
    lv_obj_set_style_text_font(_yesIconLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_label_set_text(_yesIconLabel, LV_SYMBOL_OK);
    lv_obj_align(_yesIconLabel, LV_ALIGN_TOP_MID, 0, 0);

    _yesTextLabel = lv_label_create(_yesContainer);
    lv_obj_set_style_text_font(_yesTextLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(_yesTextLabel, "Yes");
    lv_obj_align(_yesTextLabel, LV_ALIGN_BOTTOM_MID, 0, 0);

    _yesUnderline = lv_obj_create(_lvScreen);
    lv_obj_set_size(_yesUnderline, 50, 2);
    lv_obj_set_style_bg_color(_yesUnderline, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_yesUnderline, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_yesUnderline, 0, LV_PART_MAIN);
    lv_obj_align(_yesUnderline, LV_ALIGN_CENTER, -50, 56);
    lv_obj_clear_flag(_yesUnderline, LV_OBJ_FLAG_CLICKABLE);

    // No container (clickable)
    _noContainer = lv_obj_create(_lvScreen);
    lv_obj_set_size(_noContainer, 80, 56);
    lv_obj_set_style_bg_opa(_noContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_noContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_noContainer, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_noContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_noContainer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(_noContainer, LV_ALIGN_CENTER, +50, 22);
    lv_obj_add_event_cb(_noContainer, onNoClick, LV_EVENT_CLICKED, this);

    _noIconLabel = lv_label_create(_noContainer);
    lv_obj_set_style_text_font(_noIconLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_label_set_text(_noIconLabel, LV_SYMBOL_CLOSE);
    lv_obj_align(_noIconLabel, LV_ALIGN_TOP_MID, 0, 0);

    _noTextLabel = lv_label_create(_noContainer);
    lv_obj_set_style_text_font(_noTextLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(_noTextLabel, "No");
    lv_obj_align(_noTextLabel, LV_ALIGN_BOTTOM_MID, 0, 0);

    _noUnderline = lv_obj_create(_lvScreen);
    lv_obj_set_size(_noUnderline, 36, 2);
    lv_obj_set_style_bg_color(_noUnderline, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_noUnderline, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_noUnderline, 0, LV_PART_MAIN);
    lv_obj_align(_noUnderline, LV_ALIGN_CENTER, +50, 56);
    lv_obj_clear_flag(_noUnderline, LV_OBJ_FLAG_CLICKABLE);
}

void ConfirmScreen::show() {
    _selected = -1;
    if (_question) lv_label_set_text(_questionLabel, _question);
    updateHighlight();
}

// --- Input ---

void ConfirmScreen::onEncoder(int delta) {
    int next = (_selected < 0) ? (delta > 0 ? 0 : 1)
                               : ((_selected + delta + 2) % 2);
    _selected = next;
    updateHighlight();
    lv_refr_now(NULL);
}

void ConfirmScreen::onButton() {
    confirm(_selected == 0); // unset dial → false → No
}

// --- Helpers ---

void ConfirmScreen::updateHighlight() {
    bool neutral = (_selected < 0);

    lv_color_t yesColor = (neutral || _selected == 0) ? lv_color_hex(Theme::TEXT_PRIMARY)
                                                       : lv_color_hex(Theme::TEXT_DIM);
    lv_color_t noColor  = (neutral || _selected == 1) ? lv_color_hex(Theme::TEXT_PRIMARY)
                                                      : lv_color_hex(Theme::TEXT_DIM);

    lv_obj_set_style_text_color(_yesIconLabel, yesColor, LV_PART_MAIN);
    lv_obj_set_style_text_color(_yesTextLabel, yesColor, LV_PART_MAIN);
    lv_obj_set_style_text_color(_noIconLabel,  noColor,  LV_PART_MAIN);
    lv_obj_set_style_text_color(_noTextLabel,  noColor,  LV_PART_MAIN);

    if (_selected == 0) lv_obj_clear_flag(_yesUnderline, LV_OBJ_FLAG_HIDDEN);
    else                lv_obj_add_flag(_yesUnderline,   LV_OBJ_FLAG_HIDDEN);

    if (_selected == 1) lv_obj_clear_flag(_noUnderline, LV_OBJ_FLAG_HIDDEN);
    else                lv_obj_add_flag(_noUnderline,   LV_OBJ_FLAG_HIDDEN);
}

void ConfirmScreen::confirm(bool yes) {
    if (yes) { if (_onYes) _onYes(); }
    else     { if (_onNo)  _onNo();  }
}

void ConfirmScreen::onYesClick(lv_event_t* e) {
    static_cast<ConfirmScreen*>(lv_event_get_user_data(e))->confirm(true);
}

void ConfirmScreen::onNoClick(lv_event_t* e) {
    static_cast<ConfirmScreen*>(lv_event_get_user_data(e))->confirm(false);
}
