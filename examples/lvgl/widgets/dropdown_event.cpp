#include "widgets.hpp"

namespace {

using namespace widgets;

// What the label says for each city, by the city's number. The label follows `status`,
// and the dropdown's event handler writes it. The C example writes the label's text in
// its handler instead.
constexpr const char* said[] = {"Selected: Berlin", "Selected: Paris", "Selected: Tokyo",
                                "Selected: Sydney"};
constinit lv::state<const char*> status{said[0]};

void show_choice(lv::event e) {
  status = said[e.target<lv::view<"dropdown">>().get<"selected">()];
}

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = {.flex_cross_place = "center", .pad_row = tok.gap.md}})[
    lv::dropdown({.on_value_changed = &show_choice, .options = "Berlin\nParis\nTokyo\nSydney"}),
    lv::label({.text = lv::bind(status)})];

}  // namespace

void widgets::dropdown_event(lv::obj screen) {
  status = said[0];  // as the C example: every time it is built it starts at the first
  lv::place<ui>(screen);
}
