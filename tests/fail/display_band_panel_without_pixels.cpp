#include <lv/lv.hpp>
lv::band<320, 240, 24> band;
void changed(lv::area);
void f(lv::application& app) { lv::display display{app, band, &changed}; }
