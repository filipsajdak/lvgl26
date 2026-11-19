// A misspelled section is a compile error that says what is valid.
#include <lv/detail/settings.hpp>

constexpr char doc[] = R"({"alow": {"threads": false}})";
constexpr lv::settings_text text{doc, sizeof doc - 1, "test.json"};
constexpr lv::config read = lv::detail::settings_from<lv::config, text>();
