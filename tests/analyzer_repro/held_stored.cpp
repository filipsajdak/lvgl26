// Under -fanalyzer: .on_shown keeps the handle in a global after reading an id, as start_home does.
#include <lv/lv.hpp>
#include <lv/shown.hpp>

enum class page { a, b };
constinit lv::state<page> current{page::a};
constexpr auto ui = lv::column()[lv::label({.id = "title"})];
const char* seen = nullptr;
lv::held<ui> kept;
void start(lv::held<ui> p) {
  seen = lv_label_get_text(p->get<"title">().raw());
  kept = p;
}
constexpr auto window = lv::group()[lv::shown(current)[lv::is<page::a>(ui, {.on_shown = &start}), lv::otherwise()]];

int main() {
  lv::application app;
  auto root = lv::mount<window>(lv::obj{lv_screen_active()});  // C API
  return root.has_value() && seen != nullptr && kept.has_value() ? 0 : 1;
}
