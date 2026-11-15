#include <lv/lv.hpp>
constinit lv::state clicks{lv::counter{}};
constinit lv::state<bool> on{false};
// The function takes the values in the order of the dependencies: counter, then bool.
constinit lv::computed both{[](bool b, const char* c) { return b && c != nullptr; }, clicks, on};
