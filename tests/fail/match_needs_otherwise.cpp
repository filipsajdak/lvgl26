#include <lv/lv.hpp>
enum class mode { off, eco, sport };
int f(int n) { return lv::match(n)(lv::is<1> >> [] { return 0; }, lv::is<2> >> [] { return 1; }); }
