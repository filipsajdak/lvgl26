#include <lv/lv.hpp>
constinit lv::state<bool> on{false};
bool negated() {
  lv::computed off{[](bool b) { return !b; }, on.get()};
  return off.get();
}
