#include <lv/lv.hpp>
bool negated() {
  lv::computed off{[](bool b) { return !b; }, lv::state<bool>{false}};
  return off.get();
}
