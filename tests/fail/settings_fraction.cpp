// A fraction of a pixel is a compile error that says what is valid.
#include <lv/detail/settings.hpp>

constexpr char doc[] = R"({"limits": {"compile_time_image": {"value": 0.5, "unit": "px"}}})";
constexpr lv::settings_text text{doc, sizeof doc - 1, "test.json"};
constexpr lv::config read = lv::detail::settings_from<lv::config, text>();
