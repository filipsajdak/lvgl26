#include <lv/lv.hpp>
// Headers in which no doc comment has the mark, as if LVGL had reworded them.
constexpr lv::detail::doc_header reworded[] = {
    {"lvgl/widgets/lv_image.h", "/** rotation in tenths of a degree */ void lv_image_set_rotation(lv_obj_t * obj, int32_t angle);"}};
constexpr auto count = lv::detail::functions_in_tenths(reworded).size();
