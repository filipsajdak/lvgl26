#include <lv/lv.hpp>

// set() takes its properties written in place; a prepared set of properties is a function.
void f(lv::view<"arc"> arc) {
  lv::props<"arc"> p{.value = 1};
  arc.set(p);
}
