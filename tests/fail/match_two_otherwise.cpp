#include <lv/lv.hpp>
int f(int n) { return lv::match(n)(lv::otherwise.then([] { return 0; }), lv::otherwise.then([] { return 1; })); }
