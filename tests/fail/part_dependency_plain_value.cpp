#include <lv/lv.hpp>
constexpr auto shown(const lv::dependency auto& count) { return lv::label({.text = lv::bind(count, "{}")}); }
constexpr auto ui = shown(5);  // a state, not a number
