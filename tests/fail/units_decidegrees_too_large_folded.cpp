#include <lv/lv.hpp>
struct tilt { lv::decidegrees a; };
constexpr tilt t{214'748'365 * lv::deg};
