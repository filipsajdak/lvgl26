#include <lv/lv.hpp>
#include <lv/pointer.hpp>
constinit lv::frame<320, 240> frame;
lv::pointer_sample asked() { return {}; }
void run(lv::display& d) { lv::pointer<1000> mouse{d}; }
