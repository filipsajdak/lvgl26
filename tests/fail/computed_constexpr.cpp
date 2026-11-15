#include <lv/lv.hpp>
constinit lv::state<bool> on{false};
// A computed is `constinit`, like a state.
constexpr lv::computed off{[](bool b) { return !b; }, on};
