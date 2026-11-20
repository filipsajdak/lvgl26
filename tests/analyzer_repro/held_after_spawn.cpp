// Under -fanalyzer: .on_shown spawns a task on the page, then reads an id through the handle again,
// as start_home in tests/shown.cpp does.
#include <lv/lv.hpp>
#include <lv/shown.hpp>

enum class page { a, b };
constinit lv::state<page> current{page::a};
constexpr auto ui = lv::column()[lv::label({.id = "title"}), lv::button({.id = "go"})];
const char* seen = nullptr;
lv::task<> wait_click(lv::held<ui> p) { co_await p->get<"go">().next().clicked; }
void start(lv::held<ui> p) {
  lv::spawn(wait_click(p), p->root());
  seen = lv_label_get_text(p->get<"title">().raw());
}
constexpr auto window = lv::group()[lv::shown(current)[lv::is<page::a>(ui, {.on_shown = &start}), lv::otherwise()]];

int main() {
  lv::application app;
  auto root = lv::mount<window>(lv::obj{lv_screen_active()});  // C API
  return root.has_value() && seen != nullptr ? 0 : 1;
}
