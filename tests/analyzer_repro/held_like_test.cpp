// Under -fanalyzer: start_home of tests/shown.cpp as it is: a task that reads through the handle,
// spawned on the root, a counter, a read, and the handle kept in a global.
#include <lv/lv.hpp>
#include <lv/shown.hpp>

#include <string_view>

enum class page { a, b };
constinit lv::state<page> current{page::a};
constexpr auto ui = lv::column()[lv::label({.id = "title"}), lv::button({.id = "go"})];
bool saw = false;
int shown_count = 0;
std::string_view title;
lv::held<ui> kept;
lv::task<> on_click(lv::held<ui> p) {
  co_await p->get<"go">().next().clicked;
  current = page::b;
  saw = p.has_value() && lv_obj_is_valid(p->get<"title">().raw());
}
void start(lv::held<ui> p) {
  lv::spawn(on_click(p), p->root());
  ++shown_count;
  title = lv_label_get_text(p->get<"title">().raw());
  kept = p;
}
constexpr auto window = lv::group()[lv::shown(current)[lv::is<page::a>(ui, {.on_shown = &start}), lv::otherwise()]];

int main() {
  lv::application app;
  auto root = lv::mount<window>(lv::obj{lv_screen_active()});  // C API
  return root.has_value() && !title.empty() && kept.has_value() ? 0 : 1;
}
