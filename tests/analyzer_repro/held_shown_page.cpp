// Under -fanalyzer: a page of another file whose .on_shown reads an id through its lv::held.
#include <lv/lv.hpp>
#include <lv/shown.hpp>

enum class page { a, b };
constinit lv::state<page> current{page::a};
constexpr auto ui = lv::column()[lv::label({.id = "title"})];
const char* seen = nullptr;
void start(lv::held<ui> p) { seen = lv_label_get_text(p->get<"title">().raw()); }
constinit const lv::page a_page = lv::page_of<ui, {.on_shown = &start}>;
constexpr auto window = lv::group()[lv::shown(current)[lv::is<page::a>(a_page), lv::otherwise()]];

int main() {
  lv::application app;
  auto root = lv::mount<window>(lv::obj{lv_screen_active()});  // C API
  return root.has_value() && seen != nullptr ? 0 : 1;
}
