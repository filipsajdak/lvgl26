#include <lv/lv.hpp>
// The same mistake inside a larger expression, which GCC folds: see lv/detail/checked.hpp.
constexpr lv::animatable<"obj"> what{.width = {10 * lv::px, 50 * lv::pct}};
