#include <lv/lv.hpp>
#include <lv/shown.hpp>
constinit lv::state<int> level{0};
constexpr auto a = lv::label();
constexpr auto ui = lv::group()[lv::shown(level)[lv::is<0>(a), lv::otherwise()]];
