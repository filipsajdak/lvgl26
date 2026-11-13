// Proves the toolchain: LVGL links, and reflection sees its C API.
#include <lvgl/lvgl.h>

#include <meta>
#include <string_view>

namespace {

consteval int count_functions(std::string_view prefix) {
  int n = 0;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_function(m) && std::meta::has_identifier(m) &&
        std::meta::identifier_of(m).starts_with(prefix))
      ++n;
  return n;
}

static_assert(count_functions("lv_label_set_") > 0);
static_assert(count_functions("lv_no_such_widget_") == 0);

}  // namespace

int main() {
  lv_init();
  const bool ok = lv_is_initialized() && lv_version_major() >= 9;
  lv_deinit();
  return ok ? 0 : 1;
}
