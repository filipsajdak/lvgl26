#include <lv/lv.hpp>
constinit lv::state clicks{lv::counter{}};
constinit lv::computed any{[](lv::counter c) { return c.value > 0; }, clicks};
constexpr auto ui = lv::arc({.value = lv::bind(any)});
