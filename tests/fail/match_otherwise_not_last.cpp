#include <lv/lv.hpp>
enum class mode { off, eco, sport };
int f(int n) { return lv::match(n)(lv::otherwise.then([] { return 0; }), lv::is<2>.then([] { return 1; })); }
