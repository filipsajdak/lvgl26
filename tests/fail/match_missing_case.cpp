#include <lv/lv.hpp>
enum class mode { off, eco, sport };
int f(mode m) { return lv::match(m)(lv::is<mode::off>.then([] { return 0; }), lv::is<mode::eco>.then([] { return 1; })); }
