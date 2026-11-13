#include <lv/lv.hpp>
struct blob {};
int f(blob b) { return lv::match(b)(lv::otherwise.then([] { return 0; })); }
