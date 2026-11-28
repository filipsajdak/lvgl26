// The dark file leaves out a state of the group (a layer): the message says how many layers each has.
#include <lv/lv.hpp>
#include <lv/mode_look.hpp>

constexpr char day_json[] = {
#embed "mode_day.json"
    , 0};
constexpr char dark_json[] = {
#embed "mode_dark_no_layer.json"
    , 0};
constexpr auto day_tokens = lv::tokens<day_json>;
constexpr auto dark_tokens = lv::tokens<dark_json>;
struct day_design {};
struct dark_design {};
template <>
inline constexpr lv::design_of<decltype(day_tokens)> lv::design<day_design>{.tokens = day_tokens};
template <>
inline constexpr lv::design_of<decltype(dark_tokens)> lv::design<dark_design>{.tokens = dark_tokens};
template <>
inline constexpr lv::mode_designs_of<day_design, dark_design> lv::mode_designs<lv::override>{};

constexpr auto ui = lv::box({.look = lv::mode_look<"tile">});

int main() {}
