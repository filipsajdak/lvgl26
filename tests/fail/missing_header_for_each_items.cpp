// lv/lv.hpp alone: the error names the header of the name (lv/detail/missing.hpp).
#include <lv/lv.hpp>

constexpr int items[] = {1, 2};
constexpr auto parts = lv::for_each<items>([]<int I> { return lv::label(); });
