// lv::in: names of the selector that are no names.
#include <lv/animate.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

lv::task<> f(lv::view<"arc"> arc) { co_await lv::animate(lv::in<"knob">(arc), {.value = {0, 100}}, 100 * ms); }

int main() {}
