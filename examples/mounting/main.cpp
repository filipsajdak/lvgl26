// Where a mounted tree lives: in a variable, in a global, or in memory the program set
// aside. docs/mounting.md shows this file.

#include <lv/lv.hpp>

#include <array>
#include <cstddef>
#include <memory>

using namespace lv::units;

namespace {

constexpr auto home = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Home"})];
constexpr auto toast = lv::column[lv::label({.id = "message", .text = "Saved"})];
constexpr auto dialog = lv::column[lv::label({.id = "question", .text = "Discard changes?"})];

constinit lv::frame<320, 240> frame;

//! [global]
constinit lv::mounted<home> home_page;  // no widgets yet: mounted once the display exists
//! [global]

//! [memory]
// Memory set aside at start, sized by the compiler.
alignas(lv::mounted<dialog>) constinit std::array<std::byte, lv::storage_size<dialog>> dialog_memory{};
//! [memory]

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};

  //! [mount-global]
  if (auto mounted = home_page.mount(display.screen()); !mounted) return 1;
  home_page->get<"title">().set({.text = "Ready"});
  //! [mount-global]

  //! [local]
  auto shown = lv::mount<toast>(display.screen());  // built in this variable; it never moves
  if (!shown) return shown.error() == lv::mount_error::out_of_memory ? 2 : 3;
  shown->get<"message">().set({.text = "Saved at 12:00"});
  //! [local]

  //! [mount-in]
  auto& asked = lv::mount_in<dialog>(dialog_memory, display.screen());
  if (!asked) return 4;
  app.advance(50 * ms), app.step();
  std::destroy_at(&asked);  // deletes the dialog's widgets
  //! [mount-in]

  return 0;
}
