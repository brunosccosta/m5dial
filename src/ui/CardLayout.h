#pragma once
#include <lvgl.h>
#include "fonts/fa_icons.h"

// Shared layout constants and helpers for rest cards.
// All cards include this instead of hardcoding values.
//
// Three card styles:
//   Style A — Hero number: large centered value + inline unit + supporting rows (EnergyCard, WeatherNow)
//   Style B — Status + value: mode pill + arc (ACControlScreen) — uses its own layout, arc is intentional
//   Style C — Equal-weight list: 2–3 rows, same visual weight (IndoorTemps, Forecast, MeshCore)
//
// Style A and C both use makeContainer() + makeRow(). Style B is left as-is.

namespace CardLayout {

    // ── Spacing ───────────────────────────────────────────────────────────────

    // Gap between an icon glyph and its adjacent text label within a row.
    static constexpr int PAD_ICON_TEXT = 6;

    // Vertical gap between rows inside a flex column container.
    // PAD_ROW      — for hero cards (Style A): hero + 1–2 supporting rows
    // PAD_ROW_LIST — for list cards (Style C): 2–3 equal-weight rows need more breathing room
    static constexpr int PAD_ROW      = 8;
    static constexpr int PAD_ROW_LIST = 14;

    // pad_bottom applied to the outer container to offset content from true center.
    // 0 = perfectly centered at 120px. Increase to shift content upward.
    static constexpr int SHIFT_UP = 0;

    // ── Font roles ─────────────────────────────────────────────────────────────
    //
    //   Hero   (48pt) — dominant numeric value, one per card
    //   Title  (28pt) — card/section name label
    //   Value  (24pt) — secondary numeric value (e.g. room temperatures)
    //   Detail (14pt) — small labels, units, humidity, timestamps
    //   Icon         — FontAwesome solid 18pt glyphs

    inline const lv_font_t* fontHero()   { return &font_montserrat_lat_48; }
    inline const lv_font_t* fontTitle()  { return &font_montserrat_lat_28; }
    inline const lv_font_t* fontValue()  { return &font_montserrat_lat_24; }
    inline const lv_font_t* fontDetail() { return &font_montserrat_lat_14; }
    inline const lv_font_t* fontIcon()   { return &font_awesome_solid_18; }

    // ── Container helpers ──────────────────────────────────────────────────────

    // Standard 240×240 transparent outer container with vertical flex column layout.
    // All card rows are direct children of this object.
    // shiftUp: pad_bottom to push content above center (default = SHIFT_UP = 15px).
    inline lv_obj_t* makeContainer(lv_obj_t* parent, int shiftUp = SHIFT_UP, int rowGap = PAD_ROW) {
        lv_obj_t* c = lv_obj_create(parent);
        lv_obj_set_size(c, 240, 240);
        lv_obj_set_pos(c, 0, 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(c, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(c, shiftUp, LV_PART_MAIN);
        lv_obj_set_style_pad_row(c, rowGap, LV_PART_MAIN);
        lv_obj_set_layout(c, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_CLICKABLE);
        return c;
    }

    // Content-sized horizontal flex row for icon + label groups.
    // colGap: gap between child elements (default = PAD_ICON_TEXT).
    inline lv_obj_t* makeRow(lv_obj_t* parent, int colGap = PAD_ICON_TEXT) {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_column(row, colGap, LV_PART_MAIN);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        return row;
    }

    // Content-sized vertical flex column for stacking rows inside a larger row.
    // rowGap: gap between child elements (default = PAD_ROW).
    inline lv_obj_t* makeColumn(lv_obj_t* parent, int rowGap = PAD_ROW) {
        lv_obj_t* col = lv_obj_create(parent);
        lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(col, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(col, rowGap, LV_PART_MAIN);
        lv_obj_set_layout(col, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
        return col;
    }

} // namespace CardLayout
