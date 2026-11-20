// Under -fanalyzer: .on_shown calls into LVGL with the root, then reads an id through the handle.
#include <lv/lv.hpp>
#include <lv/shown.hpp>

enum class page { a, b };
constinit lv::state<page> current{page::a};
constexpr auto ui = lv::column()[lv::label({.id = "title"})];
const char* seen = nullptr;
std::uint32_t children = 0;
void start(lv::held<ui> p) {
  children = lv_obj_get_child_count(p->root().raw());
  seen = lv_label_get_text(p->get<"title">().raw());
}
constexpr auto window = lv::group()[lv::shown(current)[lv::is<page::a>(ui, {.on_shown = &start}), lv::otherwise()]];

int main() {
  lv::application app;
  auto root = lv::mount<window>(lv::obj{lv_screen_active()});  // C API
  return root.has_value() && seen != nullptr && children == 1 ? 0 : 1;
}
