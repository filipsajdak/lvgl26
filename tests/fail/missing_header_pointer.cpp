// lv/lv.hpp alone: the error names the header of the name (lv/detail/missing.hpp).
#include <lv/lv.hpp>

void f(lv::display& display) { lv::pointer mouse{display}; }
