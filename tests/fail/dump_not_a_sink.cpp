#include <lv/lv.hpp>
#include <string>
void f(lv::obj screen) {
  std::string text;
  lv::runtime<"label">::dump(screen, text);
}
