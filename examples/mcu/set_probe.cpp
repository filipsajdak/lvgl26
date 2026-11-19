// What one run-time set costs in stack on the device. Compiled with -fstack-usage in the arm-m4
// build; tools/stack_budget.py reads the result against the budget. Never linked.
#include <lv/lv.hpp>

// A value the compiler cannot see, a look, and a style written in place.
[[gnu::noinline]] void set_value(lv::view<"arc"> arc, int value) { arc.set({.value = value}); }
