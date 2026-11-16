#pragma once

// Editor stub of lv/json_writer.hpp.

#include <concepts>
#include <cstdint>
#include <string_view>

namespace lv::json {

/// Takes the pieces of a text, in order: something to call with a std::string_view.
template <class S>
concept sink = std::invocable<S&, std::string_view>;

/// Writes `text` as a JSON string, with its quotes and escapes.
template <sink S>
void write_string(S&, std::string_view) {}

/// Writes a whole number.
template <sink S>
void write_number(S&, std::int64_t) {}

}  // namespace lv::json
