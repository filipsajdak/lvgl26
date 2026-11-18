#include <lv/lv.hpp>
constexpr int not_a_tree = 1;
void f(lv::obj s) { lv::place<not_a_tree>(s); }
