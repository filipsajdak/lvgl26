// A member that the widget does not have is marked where it is written. Read by Clang
// with the editor stubs only (the test editor_unknown_member).
#include <lv/lv.hpp>

using namespace lv::units;

constexpr auto label = lv::label({.colour = 1});  // expected-error {{no matching function for call}}

// A property of another widget: a label has no range.
constexpr auto ranged = lv::label({.range = {0, 240}});  // expected-error {{no matching function for call}}

// A value of the wrong kind: a width without a unit.
constexpr auto wide = lv::label({.width = 120});  // expected-error {{no matching function for call}}

// Out of order (the id comes first), which GCC refuses.
constexpr auto ordered = lv::label({.text = "Speed", .id = "title"});  // order on purpose, expected-error {{field designators}}

constexpr auto right = lv::label({.id = "title", .text = "Speed", .width = 120 * px});
