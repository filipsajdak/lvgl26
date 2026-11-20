// Under -fanalyzer: the same read through the lv::held of an lv::one_of.
#include <lv/lv.hpp>
#include <lv/one_of.hpp>

constexpr auto ui = lv::column()[lv::label({.id = "title"})];
constexpr auto other = lv::label();
constinit lv::one_of<ui, other> slot;
const char* seen = nullptr;
void read(lv::held<ui> p) { seen = lv_label_get_text(p->get<"title">().raw()); }

int main() {
  lv::application app;
  static_cast<void>(slot.mount<ui>(lv::obj{lv_screen_active()}));  // C API
  read(slot.get<ui>());
  return seen != nullptr ? 0 : 1;
}
