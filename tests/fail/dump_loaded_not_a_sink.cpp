#include <lv/lv.hpp>
#include <lv/runtime.hpp>
#include <string>
void f(const lv::runtime<"label">::loaded& screen) {
  std::string text;
  screen.dump(text);
}
