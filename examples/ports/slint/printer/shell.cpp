// The window of the printer panel: the side bar that switches pages, the place where the page
// is shown, and the place where a sub-page slides in over it.

#include "pager.hpp"
#include "parts.hpp"
#include "queue.hpp"
#include "shell.hpp"

namespace printer {
namespace {

// ---- the side bar ----------------------------------------------------------------

template <page P>
constexpr bool differs(page now) { return now != P; }
template <page P>
constexpr bool is(page now) { return now == P; }

constinit lv::computed not_home{&differs<page::home>, active_page};
constinit lv::computed not_settings{&differs<page::settings>, active_page};
constinit lv::computed not_ink{&differs<page::ink>, active_page};
constinit lv::computed is_home{&is<page::home>, active_page};
constinit lv::computed is_settings{&is<page::settings>, active_page};
constinit lv::computed is_ink{&is<page::ink>, active_page};

template <page P>
void go() {
  active_subpage = subpage::none;
  active_page = P;
}

constexpr int icon_size = 40;
constexpr int icon_step = 64;

// A side bar icon: a rounded square. The current page's is lit.
template <const auto& Icon, class Off, class On>
constexpr auto side_icon(const Off& unlit, const On& lit, void (*go_to)(), int row) {
  return lv::group({.align = "top_mid", .size = {icon_size * px, icon_size * px}, .y = (margin + row * icon_step) * px})[
      lv::group({.hidden = lv::bind(unlit),
                 .size = {100 * pct, 100 * pct},
                 .style = {.bg_color = tok.sidebar.active, .bg_opa = tok.solid, .radius = tok.sidebar.icon_radius}}),
      lv::group({.hidden = lv::bind(lit), .size = {100 * pct, 100 * pct}})[picture<Icon>(tok.sidebar.icon)],
      lv::group({.hidden = lv::bind(unlit), .size = {100 * pct, 100 * pct}})[picture<Icon>(tok.sidebar.icon_active)],
      lv::button({.on_clicked = go_to,
                  .size = {100 * pct, 100 * pct},
                  .style = {.bg_opa = tok.clear, .shadow_width = lv::raw(0 * px)}})];
}

constexpr auto ui = lv::group({.size = {window_width * px, window_height * px},
                               .style = {.bg_color = tok.window.background, .bg_opa = tok.solid,
                                         .clip_corner = true, .radius = tok.window.radius}})[
    lv::group({.id = "main", .size = {page_width * px, 100 * pct}, .x = sidebar_width * px}),
    lv::group({.hidden = true, .id = "sub", .size = {page_width * px, 100 * pct}, .x = sidebar_width * px}),
    lv::group({.size = {sidebar_width * px, 100 * pct},
               .style = {.bg_color = tok.sidebar.background, .bg_opa = tok.solid}})[
        side_icon<icons::home>(not_home, is_home, &go<page::home>, 0),
        side_icon<icons::settings>(not_settings, is_settings, &go<page::settings>, 1),
        side_icon<icons::ink>(not_ink, is_ink, &go<page::ink>, 2)]];

// ---- the pages -------------------------------------------------------------------

pager main_pager;
slide_pager sub_pager;
lv::weak<lv::obj> sub_host;

//! [follow]
void show_page(page p) {
  main_pager.show(lv::match(p)(lv::is<page::home>.then([] { return &mount_home; }),
                               lv::is<page::settings>.then([] { return &mount_settings; }),
                               lv::is<page::ink>.then([] { return &mount_ink; })));
}

// The host of the sub-pages catches taps again, and the page slides in.
void open_subpage(make_page make) {
  if (sub_host) sub_host.get().set({.hidden = false});
  sub_pager.show(make);
}

void show_subpage(subpage s) {
  lv::match(s)(lv::is<subpage::none>.then([] { sub_pager.leave(); }),
               lv::is<subpage::print>.then([] { open_subpage(&mount_print); }),
               lv::is<subpage::scan>.then([] { open_subpage(&mount_scan); }),
               lv::is<subpage::copy>.then([] { open_subpage(&mount_copy); }),
               lv::is<subpage::usb>.then([] { open_subpage(&mount_usb); }));
}

constinit lv::effect follow_page{&show_page, active_page};
constinit lv::effect follow_subpage{&show_subpage, active_subpage};
//! [follow]

// The sub-page host stops catching taps once the page has slid out.
lv::task<> hide_when_empty(lv::obj host) {
  for (;;) {
    co_await lv::changed(active_subpage);
    if (active_subpage.get() != subpage::none) continue;
    co_await lv::sleep(tok.motion.slide_time);
    if (active_subpage.get() == subpage::none) host.set({.hidden = true});
  }
}

}  // namespace

bool start(lv::obj screen) {
  static auto window = lv::mount<ui>(screen);
  if (!window) return false;
  main_pager.host(window->get<"main">());
  sub_pager.host(window->get<"sub">());
  sub_host = lv::weak<lv::obj>{window->get<"sub">()};
  lv::spawn(hide_when_empty(window->get<"sub">()), window->get<"sub">());
  load_demo_queue();
  lv::spawn(run_queue());
  return follow_page.start() && follow_subpage.start();
}

}  // namespace printer
