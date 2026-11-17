#include <lv/lv.hpp>
int plain = 0;
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto shown() { return lv::label({.text = lv::bind(Count, "{}")}); }
constexpr auto ui = shown<plain>();  // an int is not a state
