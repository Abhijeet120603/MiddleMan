
#include "lvgl/lvgl.h"

enum { slider_r = 0, slider_g , slider_b };

typedef struct {
    uint8_t slider_type;
    lv_obj_t* label;
}rgb_mixer_t;

lv_obj_t* rect;

void slider_callback(lv_event_t* e) {
    static uint8_t r, g, b;
    lv_obj_t* slider = lv_event_get_target(e);
    rgb_mixer_t* user_data = lv_event_get_user_data(e);
    int32_t value = lv_slider_get_value(slider);
    lv_label_set_text_fmt(user_data->label, "%d", value);
    if (user_data->slider_type == slider_r) {
        r = value;
    }
    else if (user_data->slider_type == slider_g)
    { 
        g = value;
    }
    else if (user_data->slider_type == slider_b)
    {
        b = value;
    }

    lv_obj_set_style_bg_color(rect, lv_color_make(r, g, b), LV_PART_MAIN);

}

void rgb_mixer_create_ui(void)
{
    //lv_obj_t* act_scr = lv_scr_act();
    //lv_obj_t *base_obj = lv_obj_create(act_scr);
    //lv_obj_set_width(base_obj, 300);
    //lv_obj_set_height(base_obj, 50);
    //lv_obj_align(base_obj, LV_ALIGN_CENTER, 0, 0);

    static rgb_mixer_t r, g, b;
    r.slider_type = slider_r;
    g.slider_type = slider_g;
    b.slider_type = slider_b;

    lv_obj_t* slider_r = lv_slider_create(lv_scr_act());
    lv_obj_t* slider_g = lv_slider_create(lv_scr_act());
    lv_obj_t* slider_b = lv_slider_create(lv_scr_act());

    //set_range_for_sliders
    lv_slider_set_range(slider_r, 0, 255);
    lv_slider_set_range(slider_g, 0, 255);
    lv_slider_set_range(slider_b, 0, 255);

    lv_obj_align(slider_r, LV_ALIGN_CENTER, 0, -80);
    lv_obj_align_to(slider_g, slider_r, LV_ALIGN_CENTER, 0, 30);
    lv_obj_align_to(slider_b, slider_g, LV_ALIGN_CENTER, 0, 30);

    rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect, 250, 100);
    lv_obj_align_to(rect, slider_b, LV_ALIGN_CENTER, 0, 80);
    lv_obj_set_style_border_color(rect, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(rect, 2, LV_PART_MAIN);
    
    lv_obj_set_style_bg_color(slider_r, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_r, lv_palette_main(LV_PALETTE_RED), LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider_g, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_g, lv_palette_main(LV_PALETTE_GREEN), LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider_b, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_b, lv_palette_main(LV_PALETTE_BLUE), LV_PART_KNOB);

    lv_obj_t* headline = lv_label_create(lv_scr_act());
    lv_label_set_text(headline, "RGB Mixer");
    lv_obj_align(headline, LV_ALIGN_TOP_MID, 0, 10);

    r.label = lv_label_create(lv_scr_act());
    lv_label_set_text(r.label, "0");
    lv_obj_align_to(r.label,slider_r, LV_ALIGN_OUT_TOP_MID, 0, 0);

    g.label = lv_label_create(lv_scr_act());
    lv_label_set_text(g.label, "0");
    lv_obj_align_to(g.label, slider_g, LV_ALIGN_OUT_TOP_MID, 0, 0);

    b.label = lv_label_create(lv_scr_act());
    lv_label_set_text(b.label, "0");
    lv_obj_align_to(b.label, slider_b, LV_ALIGN_OUT_TOP_MID, 0, 0);

    lv_obj_add_event_cb(slider_r, slider_callback, LV_EVENT_VALUE_CHANGED, &r);
    lv_obj_add_event_cb(slider_g, slider_callback, LV_EVENT_VALUE_CHANGED, &g);
    lv_obj_add_event_cb(slider_b, slider_callback, LV_EVENT_VALUE_CHANGED, &b);

}
