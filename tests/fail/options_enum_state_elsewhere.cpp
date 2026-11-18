#include <lv/lv.hpp>
enum class sheet { a4, letter };
constinit lv::state<sheet> paper{sheet::a4};
constexpr auto ui = lv::bar({.value = lv::bind(paper)});
void f() { auto r = lv::mount<ui>(lv::screen_active()); }
