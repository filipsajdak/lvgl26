// State and bindings: values that widgets follow, values made from them, and an action that
// follows them. docs/state.md walks through this file.
//
//   state    writes state.png

#include <lv/lv.hpp>
#include <lv/png.hpp>
#include <lv/signal.hpp>

#include <mp-units/systems/si.h>

#include <type_traits>

namespace {

using namespace lv::units;
using namespace mp_units::si::unit_symbols;

//! [smallest]
constinit lv::state cups{lv::counter{2}};  // a value that widgets follow

constexpr auto cups_text = lv::label({.text = lv::bind(cups, "{} cups")});  // shows it, and follows it
//! [smallest]

//! [typed]
enum class drink { espresso, lungo, americano };  // a choice: an enum
struct strength {                                 // a value with a name: a struct of one member
  int value;
};

constinit lv::state chosen{drink::lungo};
constinit lv::state bold{strength{3}};
constinit lv::state beans{60 * g};  // a measure: a quantity with its unit
//! [typed]

//! [computed]
using grams = decltype(beans.get());

constexpr auto least_dose = 12 * g;  // the dose of the mildest cup
constexpr auto dose_step = 2 * g;    // what one step of strength adds

// Values made from states, and from each other. They store nothing; binding one makes it follow.
constinit lv::computed dose{[](strength s) { return least_dose + s.value * dose_step; }, bold};
constinit lv::computed needed{[](lv::counter c, grams d) { return c.value * d; }, cups, dose};
constinit lv::computed enough{[](grams n, grams b) { return n <= b; }, needed, beans};
//! [computed]

//! [effect]
grams grinder_dose{};  // what the grinder is set to: outside the screen

// An action that follows a value: it runs at start() and after every change of `needed`.
constinit lv::effect grinder{[](grams n) { grinder_dose = n; }, needed};
//! [effect]

//! [part]
// A count between two buttons. The part names its state as a template argument: a handler in a
// constant tree captures nothing, so each lambda names the state itself.
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto stepper(lv::tree_node auto shown) {
  return lv::row(
    {
      .flex_align = {"center", "center", "center"},
      .size       = {lv::size_content, lv::size_content},
    },
    lv::button({.on_clicked = [] { --Count; }}, lv::label({.text = "-"})),
    shown,
    lv::button({.on_clicked = [] { ++Count; }}, lv::label({.text = "+"}))
  );
}
//! [part]

//! [tree]
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
  },
  lv::dropdown({.options = lv::options_of<drink>, .selected = lv::bind(chosen), .width = 140 * px}),
  stepper<cups>(cups_text),
  lv::slider({.range = {1, 5}, .value = lv::bind(bold), .width = 200 * px}),  // follows, and writes back
  lv::label({.text = lv::bind(dose)}),                   // a quantity shows its unit: "18 g"
  lv::label({.text = lv::bind(needed, "{} g in all")}),  // a format shows the number only
  lv::label({.hidden = lv::bind(enough), .text = "Not enough beans"})
);
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame, lv::signal::dark};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  //! [start]
  grinder.start();  // runs now, and after every change of `needed`
  //! [start]
  //! [change]
  cups = lv::counter{4};  // the count, the total, the warning and the grinder follow
  //! [change]
  app.step();
  if (grinder_dose != needed.get() || enough.get()) return 2;
  return lv::write_png(display.screen(), "state.png") ? 0 : 3;
}
