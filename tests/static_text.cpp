// A constant text in a tree is not copied to LVGL's heap: the label keeps the pointer.

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstring>

namespace {

constinit lv::frame<64, 48> frame;

constexpr const char* const greeting = "Speed";
constexpr const char* const long_text = "A text that is cut with dots";

constexpr auto ui = lv::column()[
    lv::label({.id = "plain", .text = greeting}),
    lv::label({.id = "dots", .long_mode = "dots", .text = long_text}),
    lv::checkbox({.id = "box", .text = "Allow"}),
    lv::textarea({.id = "area", .text = "Notes"})];

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// The label's text is the very literal of the tree: no copy was made.
void keeps_the_pointer(lv::display& display) {
  auto root = lv::mount<ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  check(lv_label_get_text(root->get<"plain">().raw()) == greeting);
  check(lv_label_get_text(root->get<"box">().raw()) != nullptr);
}

// A widget whose text setter has no static twin (a text area) takes a copy: it edits it.
void no_twin_copies(lv::display& display) {
  auto root = lv::mount<ui>(display.screen());
  if (!root) return;
  const char* shown = lv_textarea_get_text(root->get<"area">().raw());
  check(std::strcmp(shown, "Notes") == 0);
  check(shown != static_cast<const char*>("Notes"));
}

// Static text is shown uncut by a label that cuts with dots, so that one is copied.
void dots_copy(lv::display& display) {
  auto root = lv::mount<ui>(display.screen());
  if (!root) return;
  const char* shown = lv_label_get_text(root->get<"dots">().raw());
  check(shown != long_text);
  check(std::strcmp(shown, long_text) == 0);
}

// A text set later is copied, so that its owner may go away.
void a_later_text_is_copied(lv::display& display) {
  auto root = lv::mount<ui>(display.screen());
  if (!root) return;
  char temporary[] = "Trip";
  lv_label_set_text(root->get<"plain">().raw(), temporary);
  check(lv_label_get_text(root->get<"plain">().raw()) != temporary);
  temporary[0] = 'X';
  check(std::strcmp(lv_label_get_text(root->get<"plain">().raw()), "Trip") == 0);
}

// The saving: a label with a constant text takes no more heap than one without a text.
template <const auto& Tree>
std::size_t mount_cost(lv::display& display) {
  const auto before = used_bytes();
  std::size_t cost = 0;
  {
    auto root = lv::mount<Tree>(display.screen());
    check(root.has_value());
    cost = used_bytes() - before;
  }
  check(used_bytes() == before);
  return cost;
}

constexpr auto with_text = lv::column()[lv::label({.text = "A constant text of some length"})];
constexpr auto without_text = lv::column()[lv::label({})];

void saves_the_copy(lv::display& display) {
  check(mount_cost<with_text>(display) <= mount_cost<without_text>(display));
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  keeps_the_pointer(display);
  dots_copy(display);
  no_twin_copies(display);
  a_later_text_is_copied(display);
  saves_the_copy(display);
  return failures == 0 ? 0 : 1;
}
