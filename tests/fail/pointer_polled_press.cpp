#include <lv/lv.hpp>
#include <lv/pointer.hpp>
constinit lv::frame<320, 240> frame;
lv::pointer_sample asked() { return {}; }
void run(lv::display& d) { lv::pointer panel{d, &asked}; static_cast<void>(panel.press({1, 2})); }
