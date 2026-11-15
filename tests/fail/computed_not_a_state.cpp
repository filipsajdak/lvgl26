#include <lv/lv.hpp>
constinit lv::state<bool> on{false};
constinit lv::state<bool> armed{false};
// Every dependency is the state itself; the second one here is a value.
bool both() {
  lv::computed all{[](bool a, bool b) { return a && b; }, on, armed.get()};
  return all.get();
}
