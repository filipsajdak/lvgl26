#include <lv/lv.hpp>

// A constexpr lv::props is no exception: set() takes its properties written in place. A factory
// still takes one (lv::arc(full)).
constexpr lv::props<"arc"> full{.value = 1};
void f(lv::view<"arc"> arc) { arc.set(full); }
