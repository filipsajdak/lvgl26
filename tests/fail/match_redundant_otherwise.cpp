#include <lv/lv.hpp>
enum class mode { off, eco, sport };
int f(mode m) { return lv::match(m)(lv::is<mode::off, mode::eco, mode::sport>.then([] { return 0; }), lv::otherwise.then([] { return 1; })); }
