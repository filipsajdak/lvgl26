#include <lv/lv.hpp>
enum class page { home, settings };
enum class look { light, dark };
int f(page p) { return lv::match(p)(lv::is<look::light>.then([] { return 0; }), lv::otherwise.then([] { return 1; })); }
