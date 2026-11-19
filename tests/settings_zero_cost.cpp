// A restriction changes no code: this file is compiled with no restriction and with all four
// (tests/settings/all_forbidden.hpp included first), and the two objects must have the same
// code and the same symbols. It uses none of what the restrictions forbid.
#include <lv/lv.hpp>

namespace {

constinit lv::state taps{lv::counter{}};

void tap() { ++taps; }

constexpr auto ui = lv::column[
    lv::label({.id = "count", .text = lv::bind(taps, "{} taps")}),
    lv::button({.on_clicked = &tap})[lv::label({.text = "Tap"})]];

}  // namespace

int build(lv::obj parent) {
  auto root = lv::mount<ui>(parent);
  return root.has_value() ? 0 : 1;
}
