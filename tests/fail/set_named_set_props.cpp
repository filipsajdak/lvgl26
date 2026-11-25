#include <lv/lv.hpp>

// A named set of properties of the exact type that set() takes is refused too: set() holds nothing past the call.
void f(lv::view<"arc"> arc) {
  const lv::detail::set_props<"arc"> p{.value = 1};
  arc.set(p);
}
