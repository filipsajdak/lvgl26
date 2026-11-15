#include <lv/lv.hpp>
static_assert((3'000'000'000 * lv::px).raw() != 0);
