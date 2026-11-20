#include <lv/lv.hpp>
#include <lv/shown.hpp>
enum class page { home, settings };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto b = lv::label();
void start(lv::held<b>) {}
constexpr auto ui = lv::group()[lv::shown(current)[lv::is<page::home>(a, {.on_shown = &start}), lv::otherwise()]];
void f(lv::obj parent) { auto root = lv::mount<ui>(parent); }
