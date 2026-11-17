#pragma once

// The pictures of the panel as embedded PNG files (the original's, see NOTICE.md). A page turns
// the ones it shows into pixels at compile time: lv::image_file<pictures::arrow>.

namespace treadmill::pictures {

constexpr unsigned char arrow[] = {
#embed "images/arrowIcon.png"
};
constexpr unsigned char auto_mode[] = {
#embed "images/autoModeIcon.png"
};
constexpr unsigned char manual_mode[] = {
#embed "images/manualModeIcon.png"
};
constexpr unsigned char start[] = {
#embed "images/buttonIcon.png"
};
constexpr unsigned char pause[] = {
#embed "images/pauseIcon.png"
};
constexpr unsigned char stop[] = {
#embed "images/stopIcon.png"
};
constexpr unsigned char elevation[] = {
#embed "images/elevationIcon.png"
};
constexpr unsigned char heart[] = {
#embed "images/infoPanelIcon.png"
};
constexpr unsigned char incline_light[] = {
#embed "images/inclineIconLight.png"
};
constexpr unsigned char incline[] = {
#embed "images/panelIcon.png"
};
constexpr unsigned char speed[] = {
#embed "images/sprintIconDark.png"
};
constexpr unsigned char speed_light[] = {
#embed "images/sprintIconLight.png"
};
constexpr unsigned char clock[] = {
#embed "images/paceIconDark.png"
};
constexpr unsigned char track[] = {
#embed "images/track.png"
};
constexpr unsigned char runner[] = {
#embed "images/trackView_merge_1.png"
};
constexpr unsigned char wordmark_small[] = {
#embed "images/speedtrekSmall.png"
};

// The splash screen: the slogan and the wordmark, decoded by the compiler.
constexpr unsigned char slogan[] = {
#embed "images/slogan.png"
};
constexpr unsigned char wordmark[] = {
#embed "images/speedtrekLogo.png"
};

}  // namespace treadmill::pictures
