#pragma once

// The design of Signal's fonts, for lv/signal_fonts.hpp and for the files of the library
// lvgl26::signal_type that draw them. A file that defines a font must do so before any file-level
// constant names it, and lv/signal_fonts.hpp names two (`body_font`, `label_font`): so the design
// is here, and the themes are there.

#include <lv/look.hpp>
#include <lv/signal.hpp>

namespace lv::signal {

namespace detail {

constexpr char type_json[] = {
#embed "signal/type.json"
    , 0};

}  // namespace detail

// Signal's typography: `type_tokens.font.display`, `type_tokens.text.title` and so on.
inline constexpr auto type_tokens = lv::tokens<detail::type_json, detail::signal_settings>;

// The design of Signal's fonts: the second argument of lv::font_of for a font of the library.
struct type_design {};

}  // namespace lv::signal

template <>
inline constexpr lv::design_of<decltype(lv::signal::type_tokens)> lv::design<lv::signal::type_design>{
    .tokens = lv::signal::type_tokens};
