#include <lv/lv.hpp>
enum class quality { best, medium };
enum class other { x };
constexpr auto o = lv::options_of<quality, lv::but<other::x>>;
