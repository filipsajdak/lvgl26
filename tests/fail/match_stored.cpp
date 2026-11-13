#include <lv/lv.hpp>
enum class mode { off, eco, sport };
int f(mode m) { auto later = lv::match(m); return later(lv::otherwise.then([] { return 0; })); }
