// A widget that LVGL does not have, and an id that the tree does not have, are marked
// where they are written. Read by Clang with the editor stubs only (the test
// editor_wrong_widget).
#include <lv/lv.hpp>

constexpr auto spin = lv::widget<"spinbocks">({.value = 1});  // expected-error {{constraints not satisfied}}

lv::view<"sliderr"> not_a_view;  // expected-error {{constraints not satisfied}}

constexpr auto ui = lv::column[lv::arc({.id = "speedo"}), lv::widget<"spinbox">({.id = "count"})];

void use(lv::obj screen) {
  auto root = lv::mount<ui>(screen);
  root->get<"speedo">().set({.value = 1});
  root->get<"speedometer">();  // expected-error {{no matching member function for call to 'get'}}
  root->get<"speedo">().set({.text = "no"});  // expected-error {{no matching member function for call to 'set'}}
}
