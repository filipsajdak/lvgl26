// `enabled` turns another style on and off. A constant style has no place for it.
#include <lv/lv.hpp>

constexpr auto s = lv::on("knob", {.enabled = {nullptr, true}});

int main() {}
