#include <lv/lv.hpp>
// Headers in which no doc comment has the mark, as if LVGL had reworded them.
constexpr lv::detail::doc_header reworded[] = {
    {"lvgl/widgets/lv_dropdown.h", "/** @param options a list of options */ void lv_dropdown_set_options(lv_obj_t * obj, const char * options);"}};
constexpr auto count = lv::detail::options_in(reworded).size();
