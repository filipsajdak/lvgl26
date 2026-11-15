#include <lv/lv.hpp>
constinit lv::state clicks{lv::counter{}};
constinit lv::computed doubled{[](lv::counter c) { return c.value * 2; }, clicks};
