#include <lv/lv.hpp>
#include <lv/shown.hpp>
enum class page { home, settings };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto ui = lv::group()[lv::shown(current)[lv::is<page::home>(a), lv::is<page::settings>(a)]];
void f(lv::obj parent) { auto root = lv::mount<ui>(parent); }
