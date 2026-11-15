#pragma once

// LVGL's widget examples (examples/widgets), rewritten with lvgl26.
// Each function builds its example on `screen` and leaves the widgets there, as
// the C original does. tests/examples_widgets_*.cpp check that each one renders
// the same pixels as the original.
//
// The C examples lay out the screen itself. A port puts one lv::group that fills
// the screen in its place, and the group carries the layout.

#include <lv/lv.hpp>

namespace widgets {

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};

// Every color, length and opacity of these examples.
inline constexpr auto tok = lv::tokens<tokens_json>;

// Children in the middle of a flex container, on both axes.
inline constexpr auto centered = lv::style({.flex_cross_place = LV_FLEX_ALIGN_CENTER,
                                            .flex_main_place = LV_FLEX_ALIGN_CENTER,
                                            .flex_track_place = LV_FLEX_ALIGN_CENTER});

// An opacity that the C example gives as a step of 255 (80, 120, 180). An opacity
// token is a number from 0 to 1 that is rounded to a whole percent, and no whole
// percent gives these steps, so they are the stated exception to "styles take tokens".
constexpr auto opacity_step(std::uint8_t of_255) {
  return lv::raw(lv::opa::from_raw(of_255));
}

// label
void label_bind_text(lv::obj screen);         // lv_example_label_bind_text
void label_long_mode(lv::obj screen);         // lv_example_label_long_mode
void label_recolor(lv::obj screen);           // lv_example_label_recolor
void label_set_text_newline(lv::obj screen);  // lv_example_label_set_text_newline
void label_styling(lv::obj screen);           // lv_example_label_styling
void label_text_align(lv::obj screen);        // lv_example_label_text_align

// button
void button_basic(lv::obj screen);    // lv_example_button_basic
void button_event(lv::obj screen);    // lv_example_button_event
void button_states(lv::obj screen);   // lv_example_button_states
void button_styling(lv::obj screen);  // lv_example_button_styling

}  // namespace widgets
