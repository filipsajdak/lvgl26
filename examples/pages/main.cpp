// Pages that follow a state: lv::shown shows the tree of the current page, sliding the new one in
// and the old one out, and lv::when a banner while the device is offline. The side bar sets the
// state; nothing else switches.

#include <lv/lv.hpp>
#include <lv/shown.hpp>

using namespace lv::units;

namespace {

//! [state]
enum class page { home, settings, about };
constinit lv::state<page> current{page::home};
constinit lv::state<bool> offline{false};
//! [state]

constexpr auto home =
  lv::column({.size = {240 * px, 200 * px}}, lv::label({.id = "greeting", .text = "Home"}));
constexpr auto settings = lv::column(
  {.size = {240 * px, 200 * px}}, lv::label({.text = "Settings"}), lv::slider({.id = "brightness"})
);
constexpr auto about =
  lv::column({.size = {240 * px, 200 * px}}, lv::label({.text = "Version 1.0"}));
constexpr auto offline_banner = lv::label({.text = "Offline"});

//! [on-shown]
// Called each time the home page is built, with a handle to it.
void greet(lv::held<home> page) { page->get<"greeting">().set({.text = "Welcome"}); }

// Called each time the home page comes to the front, after greet when it was just built.
int visits = 0;
void count_visit(lv::held<home>) { ++visits; }
//! [on-shown]

//! [transitions]
// Forward, to a page that comes later in the enum: the page slides in from the right, and the old
// one slides out to the left. Back, to a page that comes before it: the other way round.
constexpr auto slide_in  = lv::transition({.x = {240 * px, 0 * px}}, 250 * ms, lv::ease::out);
constexpr auto slide_out = lv::transition({.x = {0 * px, -240 * px}}, 250 * ms, lv::ease::in);
constexpr auto back_in   = lv::transition({.x = {-240 * px, 0 * px}}, 250 * ms, lv::ease::out);
constexpr auto back_out  = lv::transition({.x = {0 * px, 240 * px}}, 250 * ms, lv::ease::in);
//! [transitions]

//! [tree]
// The buttons that choose the page.
constexpr auto menu = lv::column(
  {.size = {80 * px, 240 * px}},
  lv::button({.on_clicked = [] { current = page::home; }}, lv::label({.text = "Home"})),
  lv::button({.on_clicked = [] { current = page::settings; }}, lv::label({.text = "Settings"})),
  lv::button({.on_clicked = [] { current = page::about; }}, lv::label({.text = "About"}))
);

// The page that follows `current`, and a banner above it while the device is offline.
constexpr auto pages = lv::shown(
  current, {.enter = slide_in, .leave = slide_out, .enter_back = back_in, .leave_back = back_out},
  lv::is<page::home>(home, {.on_shown = &greet, .on_front = &count_visit}),
  lv::is<page::settings>(settings), lv::is<page::about>(about)
);

constexpr auto content = lv::column(
  {.size = {240 * px, 240 * px}},
  lv::group({.size = {240 * px, 30 * px}}, lv::when(offline, offline_banner)),
  lv::group({.size = {240 * px, 200 * px}}, pages)
);

constexpr auto ui = lv::row({.size = {320 * px, 240 * px}}, menu, content);
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;

  //! [switch]
  current = page::settings;  // the settings page is built before the next frame
  offline = true;
  app.advance(50 * ms), app.step();
  //! [switch]
  current = page::about;
  lv::settle();  // or at once, where code needs the new page in the same step
  current = page::home;
  app.advance(50 * ms), app.step();
  return visits == 2 ? 0 : 2;  // home was in front at the start and once more
}
