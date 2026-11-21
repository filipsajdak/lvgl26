#include <lv/lv.hpp>
#include <lv/shown.hpp>
constinit lv::state<lv::counter> count{lv::counter{}};
constexpr auto a = lv::label();
constexpr auto ui = lv::group(lv::shown(count, lv::otherwise(a)));
