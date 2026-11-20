#include <lv/lv.hpp>
#include <lv/shown.hpp>
using namespace lv::units;
enum class page { home, settings };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto b = lv::label();
void start(lv::held<b>) {}
constinit const lv::page home_page = lv::page_of<a, {.on_shown = &start}>;
