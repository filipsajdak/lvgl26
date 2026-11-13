#include <lv/lv.hpp>
enum class mode { off, eco, sport };
int f(mode m) { return lv::match(m)(lv::is<mode::off, mode::eco> >> [] { return 0; }, lv::is<mode::eco, mode::sport> >> [] { return 1; }); }
