#include <lv/lv.hpp>
#include <lv/shown.hpp>
using namespace lv::units;
enum class page { home, settings };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto b = lv::label();
extern const lv::page home_page;
void start(lv::held<a>) {}
constexpr auto ui = lv::group()[lv::shown(current)[lv::is<page::home>(home_page, {.on_shown = &start}), lv::is<page::settings>(b)]];
void f(lv::obj parent) { auto root = lv::mount<ui>(parent); }
