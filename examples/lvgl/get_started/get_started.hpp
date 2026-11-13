#pragma once

// LVGL's "get started" examples, rewritten with lvgl26.
// Each function builds its example on `screen` and leaves the widgets there,
// as the C original does. tests/examples_get_started.cpp checks that each one
// renders the same pixels as the original.

#include <lv/lv.hpp>

namespace get_started {

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};

// Every color, radius and opacity of these examples.
inline constexpr auto tok = lv::tokens<tokens_json>;

void hello_world(lv::obj screen);  // lv_example_get_started_hello_world
void slider(lv::obj screen);       // lv_example_get_started_slider
void styles(lv::obj screen);       // lv_example_get_started_styles
void button(lv::obj screen);       // lv_example_get_started_button

}  // namespace get_started
