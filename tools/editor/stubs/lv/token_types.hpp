#pragma once

// Editor stub of lv/token_types.hpp: the composite design token types.

#include <lv/units.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace lv {

/// "cubicBezier": the two control points of an easing curve, as in CSS.
struct bezier {
  double x1 = 0, y1 = 0, x2 = 1, y2 = 1;
  friend constexpr bool operator==(const bezier&, const bezier&) = default;
};

/// "shadow". When the token is a list of shadows, this is the first one.
struct shadow_token {
  lv_color_t color{};
  length offset_x, offset_y, blur, spread;
};

/// "border".
struct border_token {
  lv_color_t color{};
  length width;
  std::string_view style;  // solid, dashed, ...
};

/// "typography": what lv::typeface makes a font from.
struct typography_token {
  std::string_view font_family;
  length font_size;
  std::int32_t font_weight = 400;
  length letter_spacing;
  double line_height = 1;  // a multiple of the font size
};

/// "transition".
struct transition_token {
  lv::duration duration, delay;
  bezier timing;
};

/// One stop of a gradient.
struct gradient_stop {
  lv_color_t color{};
  double position = 0;  // 0 to 1
};

/// "color" written #rrggbbaa: the color and its alpha, 0 to 255. A look turns the alpha into the
/// opacity beside the color (bg_color with bg_opa). The editor reads such a token as a plain color.
struct rgba_color {
  lv_color_t color{};
  std::uint8_t alpha = 255;
};

/// "gradient": up to eight stops.
struct gradient_token {
  std::array<gradient_stop, 8> stops{};
  std::size_t count = 0;
};

}  // namespace lv
