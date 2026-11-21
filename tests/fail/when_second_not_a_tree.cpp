#include <lv/lv.hpp>
#include <lv/shown.hpp>
constinit lv::state<bool> online{false};
constexpr auto ui = lv::group(lv::when(online, 5));
