module;
#include <lvgl/lvgl.h>
#include <meta>
#include <string>
#include <string_view>
export module lv;
namespace lv::detail {
consteval std::meta::info find(std::string_view name) {
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_function(m) && std::meta::has_identifier(m) && std::meta::identifier_of(m) == name) return m;
  throw "no such LVGL function";
}
template <std::size_t N> struct fixed { char s[N]; consteval fixed(const char(&a)[N]) { for (std::size_t i=0;i<N;++i) s[i]=a[i]; } consteval std::string_view sv() const { return {s, N-1}; } };
}
export namespace lv {
using obj_ptr = ::lv_obj_t*;
template <detail::fixed P, class... A> auto label_set(obj_ptr o, A... a) {
  return [: detail::find(std::string("lv_label_set_") + std::string(P.sv())) :](o, a...);
}
inline obj_ptr make_label() { ::lv_init(); return ::lv_label_create(::lv_display_get_screen_active(::lv_display_create(100,100))); }
inline const char* text(obj_ptr o) { return ::lv_label_get_text(o); }
}
