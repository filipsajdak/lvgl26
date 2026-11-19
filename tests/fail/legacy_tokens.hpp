#pragma once

// The settings of a compile-fail test that reads a token file in the forms from before the Design Tokens
// Format 2025.10 on purpose: lv::tokens<doc, legacy_tokens>.
#include <lv/lv.hpp>

struct legacy_tokens {};
template <>
inline constexpr lv::config lv::cfg<legacy_tokens>{.tokens_legacy_values = true};
