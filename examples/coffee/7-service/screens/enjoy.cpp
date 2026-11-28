// The page of the cup that is ready.

#include "enjoy.hpp"

#include "../design/design.hpp"
#include "../design/fonts.hpp"
#include "../parts/go_to.hpp"
#include "../parts/title.hpp"
#include "../state.hpp"

#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/shown.hpp>

namespace coffee {
namespace {

//! [enjoy]
constexpr auto ready = lv::label({.text = lv::bind(price), .style = body_text});

constexpr auto enjoy =
  lv::group({.style = lv::look<"screen">}, title("Enjoy"), ready, go_to<step::drinks>("Again"));
//! [enjoy]

}  // namespace

constinit const lv::page enjoy_page = lv::page_of<enjoy>;

}  // namespace coffee
