#include <lv/lv.hpp>
// A setter with an angle, declared where lvgl26 has not read the doc comments.
void lv_dial_set_rotation(lv_obj_t* dial, int32_t rotation);
constexpr auto type = lv::detail::angle_type_of(^^lv_dial_set_rotation);
