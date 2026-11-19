// The settings reader: a JSON document becomes lv::config, key by key, in any order.
// A misspelled key, a wrong type and an unknown unit are compile errors: tests/fail/settings_*.cpp.
#include <lv/lv.hpp>

#include "check.hpp"

namespace {

constexpr char empty_doc[] = "{}";
constexpr lv::settings_text empty_text{empty_doc, sizeof empty_doc - 1, "test.json"};
constexpr lv::config defaults = lv::detail::settings_from<lv::config, empty_text>();
static_assert(defaults.allow.allocation && defaults.allow.threads && defaults.allow.files &&
              defaults.allow.floating_point);
static_assert(!defaults.rules.raw_style_values && !defaults.rules.raw_integer_state);
static_assert(defaults.limits.compile_time_image.value == 128 * 128);

// The order of the keys is free, and $schema is for editors.
constexpr char full_doc[] = R"({
  "limits": {"compile_time_image": {"unit": "px", "value": 4096}},
  "$schema": "https://example.invalid/settings.schema.json",
  "rules": {"raw_integer_state": true},
  "allow": {"threads": false, "floating_point": false}
})";
constexpr lv::settings_text full_text{full_doc, sizeof full_doc - 1, "test.json"};
constexpr lv::config full = lv::detail::settings_from<lv::config, full_text>();
static_assert(!full.allow.threads && !full.allow.floating_point);
static_assert(full.allow.allocation && full.allow.files);
static_assert(full.rules.raw_integer_state && !full.rules.raw_style_values);
static_assert(full.limits.compile_time_image.value == 4096);

// Bytes, in each unit, and a fraction that gives whole bytes.
struct sizes {
  lv::byte_count stack{0};
  lv::byte_count heap{0};
  lv::byte_count image{0};
  lv::byte_count half{0};
};
constexpr char sizes_doc[] = R"({
  "stack": {"value": 4, "unit": "KiB"},
  "heap": {"value": 2, "unit": "MiB"},
  "image": {"value": 300, "unit": "B"},
  "half": {"value": 0.5, "unit": "KiB"}
})";
constexpr lv::settings_text sizes_text{sizes_doc, sizeof sizes_doc - 1, "test.json"};
constexpr sizes read_sizes = lv::detail::settings_from<sizes, sizes_text>();
static_assert(read_sizes.stack.value == 4096);
static_assert(read_sizes.heap.value == 2 * 1024 * 1024);
static_assert(read_sizes.image.value == 300);
static_assert(read_sizes.half.value == 512);

// A project with a file reads the file; a tag without one reads lv::cfg.
struct with_file {};
struct without_file {};
constexpr char project_doc[] = R"({"allow": {"files": false}})";

}  // namespace

template <>
inline constexpr lv::settings_text lv::settings_file<with_file>{project_doc, sizeof project_doc - 1, "project.json"};
template <>
inline constexpr lv::config lv::cfg<without_file>{.allow = {.threads = false}};

static_assert(!lv::detail::project<with_file>.allow.files && lv::detail::project<with_file>.allow.threads);
static_assert(!lv::detail::project<without_file>.allow.threads && lv::detail::project<without_file>.allow.files);
static_assert(lv::detail::project<lv::override>.allow.threads);

// The check of a feature passes when the project allows it, and says what is refused.
static_assert(lv::detail::refusal<^^lv::allow_settings::threads, with_file>("lv::ui_scheduler", "").empty());
static_assert(lv::detail::refusal<^^lv::allow_settings::threads, without_file>("lv::ui_scheduler", "") ==
              "lv: lv::ui_scheduler needs threads, and this project does not allow them (allow.threads).");
static_assert(lv::detail::refusal<^^lv::allow_settings::files, with_file>("lv::write_png", " Write to memory.") ==
              "lv: lv::write_png needs files, and this project does not allow them (allow.files in project.json). "
              "Write to memory.");

int main() { return failures; }
