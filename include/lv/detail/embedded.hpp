#pragma once

// A file brought into the program with #embed:
//
//   constexpr unsigned char logo_png[] = {
//   #embed "logo.png"
//   };

#include <concepts>
#include <type_traits>

namespace lv::detail {

template <class T>
concept embedded_bytes = std::is_bounded_array_v<std::remove_cvref_t<T>> &&
                         std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_cvref_t<T>>>, unsigned char>;

}  // namespace lv::detail
