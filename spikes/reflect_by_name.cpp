#include <lvgl/lvgl.h>
#include <meta>
#include <string_view>
#include <cstdio>
using namespace std::meta;
consteval int count(std::string_view prefix) {
  int n = 0;
  for (auto m : members_of(^^::, access_context::unchecked()))
    if (is_function(m) && has_identifier(m) && identifier_of(m).starts_with(prefix)) ++n;
  return n;
}
consteval info find(std::string_view name) {
  for (auto m : members_of(^^::, access_context::unchecked()))
    if (is_function(m) && has_identifier(m) && identifier_of(m) == name) return m;
  throw "no such LVGL function";
}
template <std::size_t N> struct fixed { char s[N]; consteval fixed(const char(&a)[N]) { for (std::size_t i=0;i<N;++i) s[i]=a[i]; } consteval std::string_view sv() const { return {s, N-1}; } };
template <fixed W, fixed P, class... A> auto set(lv_obj_t* o, A... a) {
  return [: find(std::string("lv_") + std::string(W.sv()) + "_set_" + std::string(P.sv())) :](o, a...);
}
static_assert(dealias(type_of(parameters_of(^^lv_label_set_long_mode)[1])) == dealias(^^lv_label_long_mode_t));
constexpr int n_all=count("lv_"), n_label=count("lv_label_set_"), n_arc=count("lv_arc_set_"), n_style=count("lv_obj_set_style_");
constexpr std::size_t n_enum=enumerators_of(dealias(^^lv_label_long_mode_t)).size();
int main() {
  std::printf("lv_=%d label_set=%d arc_set=%d obj_set_style=%d enums_long_mode=%zu\n",
    n_all, n_label, n_arc, n_style, n_enum);
  lv_init();
  lv_display_t* d = lv_display_create(100, 100);
  lv_obj_t* l = lv_label_create(lv_display_get_screen_active(d));
  set<"label", "text">(l, "hi");
  set<"label", "long_mode">(l, LV_LABEL_LONG_MODE_DOTS);
  std::printf("text=%s mode=%d\n", lv_label_get_text(l), (int)lv_label_get_long_mode(l));
}
