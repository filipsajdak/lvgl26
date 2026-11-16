#pragma once

// Editor stub of lv/detail/embedded.hpp.

#include <concepts>
#include <type_traits>

namespace lv::detail {

/// A file embedded as bytes: `constexpr unsigned char file[] = { #embed "file" };`
template <class T>
concept embedded_bytes = std::is_bounded_array_v<std::remove_cvref_t<T>> &&
                         std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_cvref_t<T>>>, unsigned char>;

}  // namespace lv::detail
