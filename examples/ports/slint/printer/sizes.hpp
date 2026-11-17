#pragma once

// The sizes of the panel, in pixels, after the original.

namespace printer {

inline constexpr int window_width = 772;
inline constexpr int window_height = 504;
inline constexpr int sidebar_width = 64;
inline constexpr int margin = 24;
inline constexpr int page_width = window_width - sidebar_width;
inline constexpr int header_height = 102;
inline constexpr int gap = 16;
inline constexpr int item_width = (page_width - gap - 2 * margin) / 2;
inline constexpr int control_height = 40;
inline constexpr int choice_height = 78;  // a name, 4 px, and a box of 52 px

}  // namespace printer
