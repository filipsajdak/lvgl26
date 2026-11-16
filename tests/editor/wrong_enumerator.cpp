// A wrong name in quotes is marked where it is written. Read by Clang with the editor
// stubs only (the test editor_wrong_enumerator); GCC reports it with lvgl26's own text.
// Clang gives two errors for each: one at the name, one at the variable it spoils.
#include <lv/lv.hpp>

// expected-error@+2 {{call to consteval function 'lv::stub::enum_arg<lv::stub::lv_align_names>}}
// expected-error@+1 {{constexpr variable 'one' must be initialized by a constant expression}}
constexpr auto one = lv::label({.align = "centre", .text = "Speed"});

// expected-error@+2 {{constexpr variable 'three' must be initialized by a constant expression}}
// expected-error@+2 {{call to consteval function 'lv::stub::enum_arg<lv::stub::lv_flex_align_names}}
constexpr auto three =
    lv::column({.flex_align = {"center", "middle", "start"}});

// expected-error@+2 {{call to consteval function 'lv::stub::enum_arg<lv::stub::lv_grad_dir_names>}}
// expected-error@+1 {{constexpr variable 'in_a_style' must be initialized by a constant expression}}
constexpr auto in_a_style = lv::style({.bg_grad_dir = "vertical"});

// Not a constant: the name is still checked.
void at_run_time(lv::view<"label"> label) {
  label.set({.align = "middel"});  // expected-error {{call to consteval function}}
  label.set({.align = "center"});
}

constexpr auto right = lv::label({.align = "center", .text = "Speed"});
