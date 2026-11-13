module;
#include <lvgl/lvgl.h>
#include <meta>
#include <array>
#include <vector>
#include <string_view>
export module lv3;
namespace lv::detail {
consteval std::vector<std::meta::info> collect(std::string_view prefix) {
  std::vector<std::meta::info> v;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_function(m) && std::meta::has_identifier(m) && std::meta::identifier_of(m).starts_with(prefix)) v.push_back(m);
  return v;
}
template <std::size_t N> struct fixed { char s[N]; consteval fixed(const char(&a)[N]) { for (std::size_t i=0;i<N;++i) s[i]=a[i]; } consteval std::string_view sv() const { return {s, N-1}; } };
// table built while the module is compiled
inline constexpr auto label_setters = std::define_static_array(collect("lv_label_set_"));
consteval std::meta::info pick(std::span<const std::meta::info> table, std::string_view prefix, std::string_view name) {
  for (auto f : table) if (std::meta::identifier_of(f).substr(prefix.size()) == name) return f;
  throw "unknown property";
}
}
export namespace lv {
struct label {
  ::lv_obj_t* p;
  template <detail::fixed P, class... A> label& set(A... a) {
    [: detail::pick(detail::label_setters, "lv_label_set_", P.sv()) :](p, a...); return *this;
  }
  const char* text() const { return ::lv_label_get_text(p); }
};
inline label make_label() { ::lv_init(); return {::lv_label_create(::lv_display_get_screen_active(::lv_display_create(100,100)))}; }
}
