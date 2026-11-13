#pragma once

// The composite design token types of the W3C Design Tokens format, as plain structs.

#include <lv/units.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace lv {

// "cubicBezier": the two control points of an easing curve.
struct bezier {
  double x1 = 0, y1 = 0, x2 = 1, y2 = 1;
  friend constexpr bool operator==(const bezier&, const bezier&) = default;
};

// "shadow". When the token is a list of shadows, this is the first one.
struct shadow_token {
  lv_color_t color{};
  length offset_x, offset_y, blur, spread;
};

// "border".
struct border_token {
  lv_color_t color{};
  length width;
  std::string_view style;  // solid, dashed, ...
};

// "typography".
struct typography_token {
  std::string_view font_family;
  length font_size;
  std::int32_t font_weight = 400;
  length letter_spacing;
  double line_height = 1;  // a multiple of the font size
};

// "transition".
struct transition_token {
  lv::duration duration, delay;
  bezier timing;
};

struct gradient_stop {
  lv_color_t color{};
  double position = 0;  // 0 to 1
};

// "gradient": up to eight stops.
struct gradient_token {
  std::array<gradient_stop, 8> stops{};
  std::size_t count = 0;
};

}  // namespace lv
