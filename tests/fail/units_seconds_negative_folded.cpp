#include <lv/lv.hpp>
struct pause { lv::duration d; };
constexpr pause p{-1 * lv::sec};
