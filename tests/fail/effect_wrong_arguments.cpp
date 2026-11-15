#include <lv/lv.hpp>
constinit lv::state clicks{lv::counter{}};
constinit lv::effect log{[](const char*) {}, clicks};
