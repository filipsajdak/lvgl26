#include <lv/lv.hpp>
constinit lv::state clicks{lv::counter{}};
constinit lv::computed half{[](lv::counter c) { return c.value / 2.0; }, clicks};
