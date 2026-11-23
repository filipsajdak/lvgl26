// The window of the printer panel: the side bar that switches pages, the place where the page
// is shown, and the place where a sub-page slides in over it.

#include <lv/image.hpp>
#include <lv/shown.hpp>

#include "pages.hpp"
#include "ui.hpp"
#include "parts.hpp"
#include "queue.hpp"
#include "shell.hpp"

namespace printer {
namespace {

// ---- the side bar ----------------------------------------------------------------

template <page P>
constexpr bool is(page now) {
  return now == P;
}

constinit lv::computed is_home{&is<page::home>, active_page};
constinit lv::computed is_settings{&is<page::settings>, active_page};
constinit lv::computed is_ink{&is<page::ink>, active_page};

template <page P>
void go() {
  active_subpage = subpage::none;
  active_page    = P;
}

constexpr int icon_size = 40;

// A side bar icon: a rounded square, lit while it is the current page's. Its picture is checked with it.
// The picture follows the page itself: a state that is set before the children exist does not trickle.
template <const auto& Icon, class Current>
constexpr auto side_icon(const Current& current, void (*go_to)()) {
  return lv::group(
    {
      .checked       = lv::bind(current),
      .size          = {icon_size * px, icon_size * px},
      .style         = {.radius = tok.sidebar.icon_radius},
      .style_checked = {.bg_color = tok.sidebar.active, .bg_opa = tok.solid},
      .on_clicked    = go_to,
    },
    lv::image({
      .align         = "center",
      .checked       = lv::bind(current),
      .src           = lv::image_file<Icon>,
      .style         = {.image_recolor = tok.sidebar.icon, .image_recolor_opa = tok.tint},
      .style_checked = {.image_recolor = tok.sidebar.icon_active},
    })
  );
}

// ---- the pages -------------------------------------------------------------------

//! [shown]
// A sub-page slides in from the right over the page, and out again.
constexpr auto slide_in =
  lv::transition({.x = {page_width * px, 0 * px}}, tok.motion.slide_time, tok.motion.slide);
constexpr auto slide_out =
  lv::transition({.x = {0 * px, page_width * px}}, tok.motion.slide_time, tok.motion.slide);

constexpr auto pages = lv::group(
  {.size = {page_width * px, 100 * pct}, .x = sidebar_width * px},
  lv::shown(
    active_page, lv::is<page::home>(home_page), lv::is<page::settings>(settings_page),
    lv::is<page::ink>(ink_page)
  )
);

// Over the page, and not clickable itself: with no sub-page shown, taps reach the page below.
constexpr auto subpages = lv::group(
  {
    .clickable = false,
    .size      = {page_width * px, 100 * pct},
    .x         = sidebar_width * px,
  },
  lv::shown(
    active_subpage, {.enter = slide_in, .leave = slide_out}, lv::is<subpage::none>(),
    lv::is<subpage::print>(print_page), lv::is<subpage::scan>(scan_page),
    lv::is<subpage::copy>(copy_page), lv::is<subpage::usb>(usb_page)
  )
);

constexpr auto sidebar = lv::group(
  {
    .flex_align = {"start", "center", "center"},
    .flex_flow  = "column",
    .size       = {sidebar_width * px, 100 * pct},
    .style = {
      .bg_color = tok.sidebar.background,
      .bg_opa   = tok.solid,
      .pad_gap  = tok.space.edge,
      .pad_top  = tok.space.edge,
    },
  },
  side_icon<icons::home>(is_home, &go<page::home>),
  side_icon<icons::settings>(is_settings, &go<page::settings>),
  side_icon<icons::ink>(is_ink, &go<page::ink>)
);

constexpr auto ui = lv::group(
  {
    .size = {window_width * px, window_height * px},
    .style = {
      .bg_color    = tok.window.background,
      .bg_opa      = tok.solid,
      .clip_corner = true,
      .radius      = tok.window.radius,
    },
  },
  pages, subpages, sidebar
);
//! [shown]

}  // namespace

bool start(lv::obj screen) {
  static auto window = lv::mount<ui>(screen);
  if (!window) return false;
  load_demo_queue();
  lv::spawn(run_queue());  // the queue runs as long as the program
  return true;
}

}  // namespace printer
