#include <lv/lv.hpp>
constinit lv::state<bool> on{false};
constexpr auto ui = lv::arc({.value = lv::bind(on)});
