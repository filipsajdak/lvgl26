#include <lv/lv.hpp>
struct tilt { lv::fine_angle a; };
constexpr tilt t{214'748'365 * lv::deg};
