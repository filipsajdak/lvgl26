#include <lv/lv.hpp>
lv::frame<320, 240> frame;
void f(lv::application& app) { lv::display display{app, frame, [](lv::area) {}}; }
