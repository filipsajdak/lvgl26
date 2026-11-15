#include <lv/lv.hpp>
constinit lv::state clicks{lv::counter{}};
constinit lv::state<bool> on{false};
// Every dependency is the state itself; the second one here is a pointer to it.
constinit lv::effect log{[](lv::counter, bool) {}, clicks, &on};
