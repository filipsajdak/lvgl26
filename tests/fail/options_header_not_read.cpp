#include <lv/lv.hpp>
void lv_dial_set_options(lv_obj_t* dial, const char* options);
constexpr bool unread = lv::detail::takes_option_list(^^lv_dial_set_options, "options");
