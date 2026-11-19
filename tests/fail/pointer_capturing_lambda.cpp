#include <lv/lv.hpp>
#include <lv/pointer.hpp>
constinit lv::frame<320, 240> frame;
lv::pointer_sample asked() { return {}; }
void run(lv::display& d, int n) { lv::pointer panel{d, [n] { return lv::pointer_sample{{n, n}, lv::pressed}; }}; }
