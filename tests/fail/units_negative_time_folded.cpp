#include <lv/lv.hpp>
// The same mistake inside a larger expression, which GCC folds: see lv/detail/checked.hpp.
struct pause { lv::duration d; };
constexpr pause p{-1 * lv::ms};
