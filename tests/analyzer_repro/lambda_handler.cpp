// Under -fanalyzer: a lambda that sets a state, as the handler of a button in a constexpr tree.
#include <lv/lv.hpp>

enum class page { a, b };
constinit lv::state<page> current{page::a};
constexpr auto ui = lv::column()[lv::button({.on_clicked = [] { current = page::b; }})];

int main() {
  lv::application app;
  auto root = lv::mount<ui>(lv::obj{lv_screen_active()});  // C API
  return root.has_value() ? 0 : 1;
}
