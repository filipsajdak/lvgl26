#include <lv/lv.hpp>
static_assert(lv::strict<int>{5'000'000'000}.raw() != 0);
