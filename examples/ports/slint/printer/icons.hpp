#pragma once

// The pictures of the panel as embedded PNG files (system-uicons, see NOTICE.md). A page turns
// the ones it shows into pixels at compile time: lv::image_file<icons::copy>.

namespace printer::icons {

constexpr unsigned char home[] = {
#embed "images/home.png"
};
constexpr unsigned char settings[] = {
#embed "images/settings.png"
};
constexpr unsigned char ink[] = {
#embed "images/ink.png"
};
constexpr unsigned char print[] = {
#embed "images/print.png"
};
constexpr unsigned char scan[] = {
#embed "images/scan.png"
};
constexpr unsigned char copy[] = {
#embed "images/copy.png"
};
constexpr unsigned char usb[] = {
#embed "images/usb.png"
};
constexpr unsigned char down[] = {
#embed "images/down.png"
};
constexpr unsigned char back[] = {
#embed "images/back.png"
};
constexpr unsigned char minus[] = {
#embed "images/minus.png"
};
constexpr unsigned char plus[] = {
#embed "images/plus.png"
};
constexpr unsigned char pause[] = {
#embed "images/pause.png"
};
constexpr unsigned char bin[] = {
#embed "images/bin.png"
};
constexpr unsigned char check[] = {
#embed "images/check.png"
};

}  // namespace printer::icons
