#include <lv/lv.hpp>
enum class quality { best, medium };
enum class sheet { a4, letter };
constinit lv::state<sheet> paper{sheet::a4};
constexpr auto ui = lv::group({})[lv::dropdown({.options = lv::options_of<quality>, .selected = lv::bind(paper)})];
void f() { auto r = lv::mount<ui>(lv::screen_active()); }
