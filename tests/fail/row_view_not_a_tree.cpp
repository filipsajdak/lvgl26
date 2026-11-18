#include <lv/lv.hpp>
constexpr int not_a_tree = 1;
void f(lv::obj row) { lv::row_view<not_a_tree>{row}; }
