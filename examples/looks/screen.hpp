#pragma once

// A small screen written with looks: the code says what each element is, tokens.json says how
// it looks. docs/looks.md walks through this file.

#include <lv/font.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/repeat.hpp>

namespace looks {

//! [registration]
constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto tok = lv::tokens<tokens_json>;
//! [registration]

}  // namespace looks

//! [design]
template <>
inline constexpr lv::design_of<decltype(looks::tok)> lv::design<lv::override>{
  .tokens = looks::tok, .type = lv::typeface({{"Barlow", 500, looks::barlow}})
};
//! [design]

namespace looks {

enum class action { none, print, scan, copy };

struct names {
  static constexpr char print[] = "Print";
  static constexpr char scan[]  = "Scan";
  static constexpr char copy[]  = "Copy";
};

//! [tree]
constexpr auto tile = []<action Action> {
  return lv::button(
    {.style = lv::look<"tile"> | lv::look_of<"tile", Action>},
    lv::label({.text = lv::like<Action, names>})
  );
};

constexpr auto ui = lv::group(
  {.style = lv::look<"page">}, lv::label({.text = "Printer", .style = lv::look<"title">}),
  lv::group({.style = lv::look<"tiles">}, lv::for_each<action, lv::but<action::none>>(tile)),
  lv::label({
    .checked = true,
    .text    = "Ready to print",
    .style   = lv::look<"status">,
  })
);
//! [tree]

}  // namespace looks
