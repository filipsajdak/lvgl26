#include <lv/lv.hpp>
constinit lv::state<bool> online{false};
auto node = lv::when(online);
