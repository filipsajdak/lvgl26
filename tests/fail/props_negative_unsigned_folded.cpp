#include <lv/lv.hpp>
// The same mistake inside a larger expression, which GCC folds: see lv/detail/checked.hpp.
struct sweep { lv::nonnegative<lv::angle> a; };
constexpr sweep s{-1 * lv::deg};
