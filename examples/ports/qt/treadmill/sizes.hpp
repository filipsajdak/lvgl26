#pragma once

// The sizes and places of the panel, in pixels, after the original (800 x 480).
// A place is named once here and used by its name in the pages.

#include <cstddef>

namespace treadmill {

struct box {
  int x;
  int y;
  int w;
  int h;
};

inline constexpr int screen_width  = 800;
inline constexpr int screen_height = 480;

// The largest picture the compiler decodes: the wordmark of the splash screen, 624 x 76.
inline constexpr std::size_t largest_picture = 624 * 76;

// ---- the main screen ----------------------------------------------------------

inline constexpr box ground{0, 64, 800, 416};
inline constexpr box nav_bar{0, 0, 800, 63};
inline constexpr box hairline{0, 63, 800, 1};
inline constexpr box button_bar{28, 364, 744, 88};
inline constexpr box wordmark{28, 21, 184, 21};
inline constexpr box divider_left{268, 378, 5, 60};
inline constexpr box divider_right{526, 378, 5, 60};

inline constexpr box heart_panel{72, 90, 120, 120};
inline constexpr box elevation_panel{608, 90, 120, 120};
inline constexpr box track_view{283, 90, 234, 253};
inline constexpr box incline_panel{30, 242, 188, 93};
inline constexpr box speed_panel{584, 243, 188, 93};

inline constexpr box start_button{57, 378, 169, 60};
inline constexpr box pause_button{315, 378, 169, 60};
inline constexpr box stop_button{573, 378, 169, 60};
inline constexpr box mode_switch{228, 18, 88, 28};

inline constexpr box theme_buttons{491, 14, 300, 37};
inline constexpr int theme_button_width  = 96;
inline constexpr int theme_button_height = 37;
inline constexpr int theme_button_step   = 102;
inline constexpr box theme_title{0, 0, 84, 27};
inline constexpr box theme_marker{28, 33, 28, 4};

// ---- parts, inside their panel ------------------------------------------------

inline constexpr box info_icon{49, 16, 22, 18};
inline constexpr box info_title{14, 37, 94, 28};
inline constexpr box info_count{31, 60, 60, 40};

inline constexpr box settings_icon{32, 70, 17, 17};
inline constexpr box settings_value{46, 0, 95, 64};
inline constexpr box settings_title{61, 66, 73, 27};
inline constexpr box settings_unit{134, 70, 54, 19};
inline constexpr box arrow_left{0, 20, 32, 28};
inline constexpr box arrow_right{156, 20, 32, 28};

inline constexpr box track_picture{1, 129, 232, 124};
inline constexpr box pace_display{61, 159, 113, 70};
inline constexpr box pace_value{21, 0, 71, 40};
inline constexpr box pace_unit{48, 47, 66, 19};
inline constexpr box pace_title{0, 43, 49, 27};
inline constexpr box time_display{15, 0, 204, 113};
inline constexpr box time_value{0, 17, 205, 96};
inline constexpr box time_title{95, 0, 47, 28};
inline constexpr box time_icon{65, 4, 20, 20};
inline constexpr int runner_size = 32;

inline constexpr int button_icon_x     = 26;  // from the inside of the button's rim
inline constexpr int button_text_x     = 76;
inline constexpr int button_text_width = 88;
inline constexpr int switch_manual_x   = 2;
inline constexpr int switch_auto_x     = 46;
inline constexpr box switch_handle{0, 2, 40, 24};
inline constexpr int switch_manual_icon_x = 13;
inline constexpr int switch_auto_icon_x   = 57;
inline constexpr int switch_icon_y        = 5;

// ---- the splash screen --------------------------------------------------------

inline constexpr int slogan_y      = 211;
inline constexpr int slogan_from   = 254;
inline constexpr int slogan_to     = 154;
inline constexpr int wordmark_y    = 109;
inline constexpr int wordmark_from = -12;
inline constexpr int wordmark_to   = 88;

// ---- the runner on the track --------------------------------------------------

inline constexpr int runner_start_x = 44;
inline constexpr int runner_start_y = 119;

}  // namespace treadmill
