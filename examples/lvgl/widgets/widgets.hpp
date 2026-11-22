#pragma once

// LVGL's widget examples (examples/widgets), rewritten with lvgl26.
// Each function builds its example on `screen` and leaves the widgets there, as
// the C original does. tests/examples_widgets_*.cpp check that each one renders
// the same pixels as the original.
//
// The C examples lay out the screen itself. A tree cannot have an existing widget
// as its root, so a port does one of two things:
// - It puts one lv::group that fills the screen in the screen's place, and the group
//   carries the layout. This is the usual way.
// - Where the picture depends on what the theme gives a screen (the gap between
//   the widgets of a row, the scrollbar), it sets the layout on the screen with
//   screen.set({...}) and places each widget as a tree of its own.

#include <lv/lv.hpp>

namespace widgets {

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};

// Every color, length and opacity of these examples.
inline constexpr auto tok = lv::tokens<tokens_json>;

// Children in the middle of a flex container, on both axes.
inline constexpr auto centered = lv::style({.flex_cross_place = "center",
                                            .flex_main_place = "center",
                                            .flex_track_place = "center"});

// Children spread evenly along a row, each in the middle of the row's height.
inline constexpr auto spread_out = lv::style({.flex_main_place = "space_evenly",
                                              .flex_track_place = "center"});

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

// arc
void arc_bind_value(lv::obj screen);     // lv_example_arc_bind_value
void arc_change_rate(lv::obj screen);    // lv_example_arc_change_rate
void arc_event(lv::obj screen);          // lv_example_arc_event
void arc_img_indicator(lv::obj screen);  // lv_example_arc_img_indicator
void arc_mode(lv::obj screen);           // lv_example_arc_mode
void arc_pie_chart(lv::obj screen);      // lv_example_arc_pie_chart
void arc_rotation(lv::obj screen);       // lv_example_arc_rotation
void arc_set_angles(lv::obj screen);     // lv_example_arc_set_angles
void arc_styling(lv::obj screen);        // lv_example_arc_styling
void arc_value_range(lv::obj screen);    // lv_example_arc_value_range

// bar
void bar_bind_value(lv::obj screen);     // lv_example_bar_bind_value
void bar_img_indicator(lv::obj screen);  // lv_example_bar_img_indicator
void bar_modes(lv::obj screen);          // lv_example_bar_modes
void bar_orientation(lv::obj screen);    // lv_example_bar_orientation
void bar_styling(lv::obj screen);        // lv_example_bar_styling
void bar_value_range(lv::obj screen);    // lv_example_bar_value_range

// slider
void slider_bind_state(lv::obj screen);               // lv_example_slider_bind_state
void slider_bind_value(lv::obj screen);               // lv_example_slider_bind_value
void slider_event(lv::obj screen);                    // lv_example_slider_event
void slider_img_indicator(lv::obj screen);            // lv_example_slider_img_indicator
void slider_modes(lv::obj screen);                    // lv_example_slider_modes
void slider_reversed_direction(lv::obj screen);       // lv_example_slider_reversed_direction
void slider_styling(lv::obj screen);                  // lv_example_slider_styling
void slider_value_range_orientation(lv::obj screen);  // lv_example_slider_value_range_orientation

// switch
void switch_event(lv::obj screen);        // lv_example_switch_event
void switch_orientation(lv::obj screen);  // lv_example_switch_orientation
void switch_state(lv::obj screen);        // lv_example_switch_state
void switch_styling(lv::obj screen);      // lv_example_switch_styling

// checkbox
void checkbox_bind_checked(lv::obj screen);  // lv_example_checkbox_bind_checked
void checkbox_event(lv::obj screen);         // lv_example_checkbox_event
void checkbox_states(lv::obj screen);        // lv_example_checkbox_states
void checkbox_styling(lv::obj screen);       // lv_example_checkbox_styling
void checkbox_text(lv::obj screen);          // lv_example_checkbox_text

// led
void led_brightness(lv::obj screen);  // lv_example_led_brightness
void led_color(lv::obj screen);       // lv_example_led_color
void led_styling(lv::obj screen);     // lv_example_led_styling

// line
void line_points(lv::obj screen);  // lv_example_line_points

// obj
void obj_bind_style(lv::obj screen);  // lv_example_obj_bind_style

// buttonmatrix
void buttonmatrix_event(lv::obj screen);  // lv_example_buttonmatrix_event
void buttonmatrix_map(lv::obj screen);    // lv_example_buttonmatrix_map

// dropdown
void dropdown_bind_value(lv::obj screen);  // lv_example_dropdown_bind_value
void dropdown_direction(lv::obj screen);   // lv_example_dropdown_direction
void dropdown_event(lv::obj screen);       // lv_example_dropdown_event
void dropdown_text(lv::obj screen);        // lv_example_dropdown_text

// roller
void roller_bind_value(lv::obj screen);    // lv_example_roller_bind_value
void roller_event(lv::obj screen);         // lv_example_roller_event
void roller_options(lv::obj screen);       // lv_example_roller_options
void roller_styling(lv::obj screen);       // lv_example_roller_styling
void roller_visible_rows(lv::obj screen);  // lv_example_roller_visible_rows

// spinbox
void spinbox_bind_value(lv::obj screen);  // lv_example_spinbox_bind_value
void spinbox_format(lv::obj screen);      // lv_example_spinbox_format
void spinbox_rollover(lv::obj screen);    // lv_example_spinbox_rollover
void spinbox_styling(lv::obj screen);     // lv_example_spinbox_styling
void spinbox_value(lv::obj screen);       // lv_example_spinbox_value

// textarea
void textarea_placeholder(lv::obj screen);  // lv_example_textarea_placeholder

// image
void image_inner_align(lv::obj screen);     // lv_example_image_inner_align
void image_src(lv::obj screen);             // lv_example_image_src
void image_transformations(lv::obj screen);  // lv_example_image_transformations

// scale
void scale_modes(lv::obj screen);    // lv_example_scale_modes
void scale_range(lv::obj screen);    // lv_example_scale_range
void scale_styling(lv::obj screen);  // lv_example_scale_styling
void scale_ticks(lv::obj screen);    // lv_example_scale_ticks

// keyboard
void keyboard_modes(lv::obj screen);     // lv_example_keyboard_modes
void keyboard_popovers(lv::obj screen);  // lv_example_keyboard_popovers
void keyboard_textarea(lv::obj screen);  // lv_example_keyboard_textarea

}  // namespace widgets
