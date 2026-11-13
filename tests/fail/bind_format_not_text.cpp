#include <lv/lv.hpp>
constinit lv::state<int> n{0};
constexpr auto ui = lv::arc({.value = lv::bind(n, "%d")});
