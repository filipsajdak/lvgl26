#pragma once

// Exo, the font of the original (SIL Open Font License 1.1, see NOTICE.md and fonts/OFL.txt).
// The compiler draws a font where it is asked for. The big numbers hold digits only, so they
// cost little; the words hold the letters the screen shows.
//
//   constexpr auto label_font = treadmill::words_face(tok.text.label);

#include <lv/font.hpp>
#include <lv/lv.hpp>

namespace treadmill {

constexpr unsigned char exo_medium[] = {
#embed "fonts/Exo-Medium.ttf"
};
constexpr unsigned char exo_semibold[] = {
#embed "fonts/Exo-SemiBold.ttf"
};

// What the screens say in Medium: capitals for the titles, a few lower-case letters for the
// names of the looks and for "Time", and the brackets of a unit and the space.
constexpr char words[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz()%/";
// What a reading shows: digits, a colon for a time and a point for a speed.
constexpr char digits[] = "0123456789:.";
// What the buttons say in SemiBold: START, PAUSE, STOP.
constexpr char capitals[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

constexpr auto words_face    = lv::typeface({{"Exo", 500, exo_medium, words}});
constexpr auto digits_face   = lv::typeface({{"Exo", 600, exo_semibold, digits}});
constexpr auto capitals_face = lv::typeface({{"Exo", 600, exo_semibold, capitals}});

}  // namespace treadmill
