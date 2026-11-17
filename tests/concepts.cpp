// The concepts a user writes a reusable part with, and what satisfies each (docs/concepts.md).
// Nothing runs: it compiles or it does not.

#include <lv/lv.hpp>

#include <mp-units/systems/si.h>

#include <concepts>
#include <string>

namespace {

using namespace lv::units;
using namespace mp_units::si::unit_symbols;

void function() {}
void with_event(lv::event) {}
constexpr auto no_captures = [] {};
constexpr auto event_no_captures = [](lv::event) {};
int count_ = 0;
auto one_pointer = [p = &count_] { ++*p; };
struct callable_object {
  void operator()() const {}
};

// lv::event_handler: a function, a lambda or an object that takes an event or nothing.
static_assert(lv::event_handler<void (*)()>);
static_assert(lv::event_handler<void (*)(lv::event)>);
static_assert(lv::event_handler<decltype(function)>);
static_assert(lv::event_handler<decltype(with_event)>);
static_assert(lv::event_handler<decltype(no_captures)>);
static_assert(lv::event_handler<decltype(event_no_captures)>);
static_assert(lv::event_handler<decltype(one_pointer)>);
static_assert(lv::event_handler<callable_object>);
static_assert(!lv::event_handler<int>);
static_assert(!lv::event_handler<const char*>);
static_assert(!lv::event_handler<void (*)(int)>);
static_assert(!lv::event_handler<decltype([](int, int) {})>);

// lv::tree_handler: what a constexpr tree can hold, so no capture at all.
static_assert(lv::tree_handler<void (*)()>);
static_assert(lv::tree_handler<void (*)(lv::event)>);
static_assert(lv::tree_handler<decltype(function)>);
static_assert(lv::tree_handler<decltype(with_event)>);
static_assert(lv::tree_handler<decltype(no_captures)>);
static_assert(lv::tree_handler<decltype(event_no_captures)>);
static_assert(!lv::tree_handler<decltype(one_pointer)>);  // a capture of one pointer is for view.on<>()
static_assert(!lv::tree_handler<callable_object>);        // an object has state, even if empty
static_assert(!lv::tree_handler<int>);
static_assert(!lv::tree_handler<void (*)(int)>);
static_assert(lv::event_handler<decltype(one_pointer)> && lv::event_handler<callable_object>);

// lv::dependency: a state or a computed, never a value or a pointer to one.
constinit lv::state<lv::counter> clicks{lv::counter{}};
constinit lv::state<bool> on{false};
constinit lv::state<bool> armed{false};
inline const lv::computed both{[](bool a, bool b) { return a && b; }, on, armed};
static_assert(lv::dependency<decltype(clicks)>);
static_assert(lv::dependency<decltype(on)>);
static_assert(lv::dependency<std::remove_cvref_t<decltype(both)>>);
static_assert(!lv::dependency<bool>);
static_assert(!lv::dependency<lv::counter>);
static_assert(!lv::dependency<lv::state<bool>*>);
static_assert(!lv::dependency<int>);

// lv::state_value: what a state can hold.
static_assert(lv::state_value<bool>);
static_assert(lv::state_value<lv::counter>);
static_assert(lv::state_value<lv_color_t>);
static_assert(lv::state_value<const char*>);
static_assert(lv::state_value<mp_units::quantity<km / h, int>>);
static_assert(!lv::state_value<double>);
static_assert(!lv::state_value<std::string>);
static_assert(!lv::state_value<mp_units::quantity<km / h, double>>);

// lv::tree_node: what lv::button(...) and a part make, also by reference.
constexpr auto leaf = lv::label({.text = "A"});
constexpr auto parent = lv::group({})[leaf];
static_assert(lv::tree_node<decltype(leaf)>);
static_assert(lv::tree_node<const decltype(parent)&>);
static_assert(!lv::tree_node<int>);
static_assert(!lv::tree_node<lv::obj>);
static_assert(!lv::tree_node<decltype(clicks)>);

// lv::view_value: a view of a widget.
static_assert(lv::view_value<lv::obj>);
static_assert(lv::view_value<lv::view<"label">>);
static_assert(!lv::view_value<lv_obj_t*>);
static_assert(!lv::view_value<decltype(leaf)>);

// lv::style_value: the type of `.style`.
constexpr auto card = lv::style({.radius = lv::raw(4 * px)});
static_assert(lv::style_value<decltype(card)>);
static_assert(lv::style_value<std::remove_cvref_t<const decltype(card)&>>);
static_assert(!lv::style_value<int>);
static_assert(!lv::style_value<lv_color_t>);

// lv::token_value and lv::token_of.
static_assert(lv::token_value<lv::token<lv_color_t>>);
static_assert(lv::token_value<const lv::token<lv::length>&>);
static_assert(lv::token_of<lv::token<lv_color_t>, lv_color_t>);
static_assert(lv::token_of<const lv::token<lv::length>&, lv::length>);
static_assert(!lv::token_of<lv::token<lv_color_t>, lv::length>);
static_assert(!lv::token_value<lv_color_t>);
static_assert(!lv::token_value<lv::raw_value<lv_color_t>>);  // a reasoned exception, not a token
static_assert(!lv::token_value<int>);
struct colour_pair {
  lv::token<lv_color_t> from;
  lv::token<lv_color_t> to;
};
static_assert(!lv::token_value<colour_pair>);  // a group of tokens is not one

// lv::font_value: what `.text_font` takes.
static_assert(lv::font_value<lv::token<const lv_font_t*>>);
static_assert(lv::font_value<const lv::token<const lv_font_t*>&>);
static_assert(lv::font_value<lv::raw_value<const lv_font_t*>>);
static_assert(lv::font_value<lv::font_binding>);
static_assert(!lv::font_value<const lv_font_t*>);  // not a design value
static_assert(!lv::font_value<lv::token<lv_color_t>>);
static_assert(!lv::font_value<lv::token<lv::typography_token>>);  // a recipe: lv::typeface makes the font
static_assert(!lv::font_value<int>);

// The names of the library's own code are the public ones.
static_assert(std::is_same_v<lv::view<"obj">, lv::obj>);

}  // namespace

int main() {}
