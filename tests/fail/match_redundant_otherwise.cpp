#include <lv/lv.hpp>
enum class mode { off, eco, sport };
int f(mode m) { return lv::match(m)(lv::is<mode::off, mode::eco, mode::sport> >> [] { return 0; }, lv::otherwise >> [] { return 1; }); }
