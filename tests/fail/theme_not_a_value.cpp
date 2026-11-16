// `enabled` turns another style of a widget on and off. It is not a value that a
// style holds, so a theme has no place for it.
#include <lv/lv.hpp>

constexpr auto mine = lv::theme({.button = {.style = {.enabled = {nullptr, true}}}});

int main() {}
