#include "EnergyPriceCard.h"
#include "../../AppState.h"
#include "../Theme.h"
#include "../CardLayout.h"

// tariff_group → colour: 0 low (green), 1 normal (amber), 2 high (red)
static lv_color_t groupColor(uint8_t group) {
    switch (group) {
        case 2:  return lv_color_hex(Theme::ACCENT_ERROR);
        case 1:  return lv_color_hex(Theme::TEMP_MID);
        default: return lv_color_hex(Theme::AC_MODE_AUTO);
    }
}

// Recolour each bar by its tariff group. Fired per draw task because the chart
// has LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS set. For LV_PART_ITEMS, base->id2 is the
// point index within the series.
static void chartDrawCb(lv_event_t* e) {
    lv_draw_task_t* task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t* base = (lv_draw_dsc_base_t*)lv_draw_task_get_draw_dsc(task);
    if (base->part != LV_PART_ITEMS) return;

    lv_draw_fill_dsc_t* fill = lv_draw_task_get_fill_dsc(task);
    if (!fill) return;

    uint32_t idx = base->id2;
    if ((int)idx >= appState.price.count) return;
    fill->color = groupColor(appState.price.pts[idx].group);
}

void EnergyPriceCard::init(lv_obj_t* parent) {
    _container = CardLayout::makeContainer(parent);

    // Hero row: current price + unit
    lv_obj_t* heroRow = CardLayout::makeRow(_container);
    _nowLabel = lv_label_create(heroRow);
    lv_obj_set_style_text_font(_nowLabel, CardLayout::fontTitle(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_nowLabel, lv_color_hex(Theme::TEXT_PRIMARY), LV_PART_MAIN);
    lv_label_set_text(_nowLabel, "--");
    _unitLabel = lv_label_create(heroRow);
    lv_obj_set_style_text_font(_unitLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_unitLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_unitLabel, "€/kWh");

    // Bar chart
    _chart = lv_chart_create(_container);
    lv_obj_set_size(_chart, 170, 72);
    lv_chart_set_type(_chart, LV_CHART_TYPE_BAR);
    lv_chart_set_div_line_count(_chart, 0, 0);
    lv_chart_set_update_mode(_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_obj_set_style_bg_opa(_chart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_chart, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_chart, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(_chart, 1, LV_PART_MAIN); // tight bars
    lv_obj_set_style_radius(_chart, 0, LV_PART_ITEMS);
    lv_obj_clear_flag(_chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(_chart, chartDrawCb, LV_EVENT_DRAW_TASK_ADDED, nullptr);
    _ser = lv_chart_add_series(_chart, lv_color_hex(Theme::AC_MODE_AUTO),
                               LV_CHART_AXIS_PRIMARY_Y);

    // Peak callout
    _peakLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_peakLabel, CardLayout::fontDetail(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_peakLabel, lv_color_hex(Theme::TEXT_DIM), LV_PART_MAIN);
    lv_label_set_text(_peakLabel, "");
}

void EnergyPriceCard::update() {
    PriceState& p = appState.price;
    if (!p.valid || p.count == 0) return;

    char buf[32];

    // Hero: current hour price (pts[0]), coloured by its group
    snprintf(buf, sizeof(buf), "%.2f", p.pts[0].price);
    lv_label_set_text(_nowLabel, buf);
    lv_obj_set_style_text_color(_nowLabel, groupColor(p.pts[0].group), LV_PART_MAIN);

    // Chart: one bar per forecast hour, value in milli-€/kWh
    lv_chart_set_point_count(_chart, p.count);
    int top = (int)(p.peakPrice * 1000.0f * 1.15f);
    if (top < 100) top = 100;
    lv_chart_set_range(_chart, LV_CHART_AXIS_PRIMARY_Y, 0, top);
    for (int i = 0; i < p.count; i++) {
        lv_chart_set_value_by_id(_chart, _ser, i, (int)(p.pts[i].price * 1000.0f));
    }
    lv_chart_refresh(_chart);

    // Peak callout — only meaningful if the peak is ahead of the current hour
    if (p.peakIdx > 0) {
        snprintf(buf, sizeof(buf), "Peak %.2f @ %02d:00", p.peakPrice, p.peakHourLocal);
        lv_label_set_text(_peakLabel, buf);
        lv_obj_set_style_text_color(_peakLabel,
            p.peakIsHigh ? lv_color_hex(Theme::ACCENT_ERROR) : lv_color_hex(Theme::TEXT_DIM),
            LV_PART_MAIN);
    } else {
        lv_label_set_text(_peakLabel, "No spike ahead");
        lv_obj_set_style_text_color(_peakLabel, lv_color_hex(Theme::TEXT_FAINT), LV_PART_MAIN);
    }
}

bool EnergyPriceCard::isVisible() const {
    return appState.price.valid;
}

void EnergyPriceCard::show() { lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN); update(); }
void EnergyPriceCard::hide() { lv_obj_add_flag(_container,   LV_OBJ_FLAG_HIDDEN); }
