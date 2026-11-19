#include <lv/lv.hpp>

using namespace lv::units;

// set() holds a style by reference, so a style written in place is made with lv::style({...}).
void f(lv::view<"obj"> box) { box.set({.style = {.radius = lv::raw(2 * px)}}); }
