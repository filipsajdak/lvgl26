#pragma once

// The sizes of the panel, in pixels, after the original.

namespace printer {

inline constexpr int window_width  = 772;
inline constexpr int window_height = 504;
inline constexpr int sidebar_width = 64;
inline constexpr int margin        = 24;
inline constexpr int page_width    = window_width - sidebar_width;
inline constexpr int header_height = 102;
inline constexpr int plain_header_height =
  86;  // the header of Settings and Ink, which have no way back
inline constexpr int gap             = 16;
inline constexpr int item_width      = (page_width - gap - 2 * margin) / 2;
inline constexpr int tile_width      = 162;  // the four actions of the home page
inline constexpr int tile_height     = 192;
inline constexpr int control_height  = 40;
inline constexpr int spin_name_width = 90;  // the name of a spin box, up to its fewer button
inline constexpr int spin_value_width =
  52;  // the number between its two buttons, with a gap before it
inline constexpr int choice_height = 78;  // a name, 4 px, and a box of 52 px

}  // namespace printer
