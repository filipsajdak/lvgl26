// A negative limit is a compile error that says what is valid.
#include <lv/detail/json_fault.hpp>  // as the header that lvgl26_settings() writes does
#include <lv/detail/settings.hpp>

constexpr char doc[] = R"({"limits": {"compile_time_image": {"value": -1, "unit": "px"}}})";
constexpr lv::settings_text text{doc, sizeof doc - 1, "test.json"};
constexpr lv::config read = lv::detail::settings_from<lv::config, text>();
