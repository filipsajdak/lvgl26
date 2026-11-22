// Reusable parts: functions that return a piece of tree and constrain their parameters
// with the public concepts. docs/concepts.md walks through this file.
//
//   parts    builds the screen; tests/handlers.cpp presses the buttons of such parts

#include <lv/lv.hpp>
#include <lv/signal_type.hpp>

using namespace lv::units;

namespace {

constexpr auto text = lv::signal::type_tokens.text;
constexpr auto title_font = lv::signal::type(text.title);

//! [state]
constinit lv::state copies{lv::counter{1}};
constinit lv::state pages{lv::counter{1}};

void fewer_copies() { copies.update([](lv::counter c) { return --c; }); }
void more_copies() { copies.update([](lv::counter c) { return ++c; }); }
//! [state]

//! [parts]
// A heading in a font that a style accepts: a typeface's font, lv::raw(font) or lv::bind(font).
constexpr auto heading(const char* name, const lv::font_value auto& font) {
  return lv::label({.text = name, .style = {.text_font = font}});
}

// A number with a button on each side. The state is any state or computed; the handlers are
// functions or lambdas that capture nothing.
constexpr auto spin_box(const char* name, const lv::dependency auto& count, lv::tree_handler auto fewer,
                        lv::tree_handler auto more) {
  return lv::row({.flex_align = {"start", "center", "center"}, .size = {300 * px, lv::size_content}})[
      lv::label({.flex_grow = 1, .text = name}),
      lv::button({.on_clicked = fewer})[lv::label({.text = "-"})],
      lv::label({.text = lv::bind(count, "{}")}),
      lv::button({.on_clicked = more})[lv::label({.text = "+"})]];
}

// The same with the state as a template argument: the lambdas name it, and the part needs
// no handler parameters.
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto counting_spin_box(const char* name) {
  return spin_box(name, Count, [] { Count.update([](lv::counter c) { return --c; }); },
                  [] { Count.update([](lv::counter c) { return ++c; }); });
}

// A box around any piece of tree.
constexpr auto card(lv::tree_node auto content) {
  return lv::box({.flex_flow = "column", .size = {320 * px, lv::size_content}})[content];
}
//! [parts]

//! [tree]
constexpr auto ui = card(lv::column({})[
    heading("Print", title_font),
    spin_box("Copies", copies, &fewer_copies, &more_copies),
    counting_spin_box<pages>("Pages")]);
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame, lv::signal::dark_with_type};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  return 0;
}
