// Where a mounted tree lives: in a variable, or in a global that is mounted and unmounted
// as its window opens and closes. docs/mounting.md shows this file.

#include <lv/lv.hpp>

using namespace lv::units;

namespace {

constexpr auto home = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Home"})];
constexpr auto toast = lv::column[lv::label({.id = "message", .text = "Saved"})];
constexpr auto dialog = lv::column[lv::label({.id = "question", .text = "Discard changes?"})];

constinit lv::frame<320, 240> frame;

//! [global]
constinit lv::mounted<home> home_page;  // no widgets yet: mounted once the display exists
constinit lv::mounted<dialog> asked;    // a window that opens and closes
//! [global]

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};

  //! [mount-global]
  if (!home_page.mount(display.screen())) return 1;
  home_page->get<"title">().set({.text = "Ready"});
  //! [mount-global]

  //! [local]
  auto shown = lv::mount<toast>(display.screen());  // built in this variable; it never moves
  if (!shown.has_value()) return shown.error() == lv::mount_error::out_of_memory ? 2 : 3;
  shown->get<"message">().set({.text = "Saved at 12:00"});
  //! [local]

  //! [window]
  if (!asked.mount(display.screen())) return 4;  // the window opens
  app.advance(50 * ms), app.step();
  asked.unmount();                               // it closes: its widgets go
  if (!asked.mount(display.screen())) return 5;  // and opens again, in the same memory
  //! [window]

  return 0;
}
