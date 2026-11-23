// Where a mounted tree lives: in a variable, in a global that is mounted and unmounted as
// its window opens and closes, or in a slot for one of several trees. docs/mounting.md
// shows this file.

#include <lv/lv.hpp>
#include <lv/one_of.hpp>

using namespace lv::units;

namespace {

constexpr auto home =
  lv::column({.size = {320 * px, 240 * px}}, lv::label({.id = "title", .text = "Home"}));
constexpr auto toast  = lv::column(lv::label({.id = "message", .text = "Saved"}));
constexpr auto dialog = lv::column(lv::label({.id = "question", .text = "Discard changes?"}));

constexpr auto saving = lv::column(lv::label({.id = "progress", .text = "Saving"}));
constexpr auto failed = lv::column(lv::label({.id = "reason", .text = "Not saved"}));

constinit lv::frame<320, 240> frame;

//! [slot]
constinit lv::one_of<saving, failed> status;  // a window that shows one of two dialogs
//! [slot]

//! [global]
constinit lv::mounted<home> home_page;  // no widgets yet: mounted once the display exists
constinit lv::mounted<dialog> asked;    // a window that opens and closes
//! [global]

// A page in a variable, a global page, and a window that opens and closes.
int pages(lv::application& app, lv::display& display) {
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

// A window that shows one of two dialogs.
int dialogs(lv::application& app, lv::display& display) {
  //! [slot-use]
  if (!status.mount<saving>(display.screen())) return 6;
  app.advance(50 * ms), app.step();
  if (!status.mount<failed>(display.screen())) return 7;  // saving ends first, in the same memory
  status.visit(
    [](lv::held<failed> f) { f->get<"reason">().set({.text = "The card is full"}); },
    [](const auto&) {}
  );  // saving has nothing to update
  //! [slot-use]
  return status.holds<failed>() ? 0 : 8;
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  const int result = pages(app, display);
  return result != 0 ? result : dialogs(app, display);
}
