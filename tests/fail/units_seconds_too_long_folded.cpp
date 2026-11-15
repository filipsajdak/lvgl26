#include <lv/lv.hpp>
struct pause { lv::duration d; };
constexpr pause p{2'000'001 * lv::sec};
