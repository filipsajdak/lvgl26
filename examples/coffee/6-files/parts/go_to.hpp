#pragma once

// A button that takes the machine to another step.

#include "../state.hpp"

#include <lv/lv.hpp>

namespace coffee {

template <step To>
constexpr auto go_to(const char* text) {
  return lv::button({.on_clicked = [] { current = To; }}, lv::label({.text = text}));
}

}  // namespace coffee
