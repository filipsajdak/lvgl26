module;
#include <lvgl/lvgl.h>
#include <meta>
#include <string>
#include <string_view>
export module lv2;
namespace lv::detail {
consteval std::meta::info find(std::string_view name) {
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_function(m) && std::meta::has_identifier(m) && std::meta::identifier_of(m) == name) return m;
  throw "no such LVGL function";
}
}
export namespace lv {
using obj_ptr = ::lv_obj_t*;
// reflection resolved while compiling the module, exported as a constant
inline constexpr auto label_set_text = &[: detail::find("lv_label_set_text") :];
struct label { obj_ptr p; label& text(const char* s) { label_set_text(p, s); return *this; } const char* text() const { return ::lv_label_get_text(p); } };
inline label make_label() { ::lv_init(); return {::lv_label_create(::lv_display_get_screen_active(::lv_display_create(100,100)))}; }
}
