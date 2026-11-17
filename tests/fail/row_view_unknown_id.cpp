#include <lv/lv.hpp>
constexpr auto card = lv::row[lv::label({.id = "title"})];
void f(lv::obj row) { lv::row_view<card>{row}.get<"titel">(); }
