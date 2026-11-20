// Pages that follow a state: lv::shown shows the tree of the current page, and lv::when a
// banner while the device is offline. The side bar sets the state; nothing else switches.

#include <lv/lv.hpp>
#include <lv/shown.hpp>

using namespace lv::units;

namespace {

//! [state]
enum class page { home, settings, about };
constinit lv::state<page> current{page::home};
constinit lv::state<bool> offline{false};
//! [state]

constexpr auto home = lv::column({.size = {240 * px, 200 * px}})[
    lv::label({.id = "greeting", .text = "Home"})];
constexpr auto settings = lv::column({.size = {240 * px, 200 * px}})[
    lv::label({.text = "Settings"}), lv::slider({.id = "brightness"})];
constexpr auto about = lv::column({.size = {240 * px, 200 * px}})[lv::label({.text = "Version 1.0"})];
constexpr auto offline_banner = lv::label({.text = "Offline"});

//! [on-shown]
// Called each time the home page is built, with a handle to it.
void greet(lv::held<home> page) { page->get<"greeting">().set({.text = "Welcome back"}); }
//! [on-shown]

template <page P>
void go() {
  current = P;
}

//! [tree]
constexpr auto ui = lv::row({.size = {320 * px, 240 * px}})[
    lv::column({.size = {80 * px, 240 * px}})[
        lv::button({.on_clicked = &go<page::home>})[lv::label({.text = "Home"})],
        lv::button({.on_clicked = &go<page::settings>})[lv::label({.text = "Settings"})],
        lv::button({.on_clicked = &go<page::about>})[lv::label({.text = "About"})]],
    lv::column({.size = {240 * px, 240 * px}})[
        lv::group({.size = {240 * px, 30 * px}})[lv::when(offline)[offline_banner]],
        lv::group({.size = {240 * px, 200 * px}})[
            lv::shown(current)[
                lv::is<page::home>(home, {.on_shown = &greet}),
                lv::is<page::settings>(settings),
                lv::is<page::about>(about)]]]];
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
  return 0;
}
