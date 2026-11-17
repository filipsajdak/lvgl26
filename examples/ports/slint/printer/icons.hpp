#pragma once

// The pictures of the panel as embedded PNG files (system-uicons, see NOTICE.md). A page turns
// the ones it shows into pixels at compile time: lv::image_file<icons::copy>.

namespace printer::icons {

inline constexpr unsigned char home[] = {
#embed "images/home.png"
};
inline constexpr unsigned char settings[] = {
#embed "images/settings.png"
};
inline constexpr unsigned char ink[] = {
#embed "images/ink.png"
};
inline constexpr unsigned char print[] = {
#embed "images/print.png"
};
inline constexpr unsigned char scan[] = {
#embed "images/scan.png"
};
inline constexpr unsigned char copy[] = {
#embed "images/copy.png"
};
inline constexpr unsigned char usb[] = {
#embed "images/usb.png"
};
inline constexpr unsigned char down[] = {
#embed "images/down.png"
};
inline constexpr unsigned char back[] = {
#embed "images/back.png"
};
inline constexpr unsigned char minus[] = {
#embed "images/minus.png"
};
inline constexpr unsigned char plus[] = {
#embed "images/plus.png"
};
inline constexpr unsigned char pause[] = {
#embed "images/pause.png"
};
inline constexpr unsigned char bin[] = {
#embed "images/bin.png"
};
inline constexpr unsigned char check[] = {
#embed "images/check.png"
};

}  // namespace printer::icons
