#include <lv/lv.hpp>
#include <lv/pointer.hpp>
constinit lv::frame<320, 240> frame;
lv::pointer_sample asked() { return {}; }
void run(lv::application& app) { lv::pointer mouse{lv::display{app, frame}}; }
