#pragma once

// Editor stub of lv/detail/json_fault.hpp, which the header written by lvgl26_settings() includes.

#include <string>
#include <string_view>

namespace lv::detail {

/// The sentence that says where a text stops being JSON.
inline std::string json_fault_text(std::string_view) { __builtin_unreachable(); }

}  // namespace lv::detail
