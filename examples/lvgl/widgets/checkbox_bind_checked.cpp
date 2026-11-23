#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Whether the box is ticked, and whether the note below it is hidden.
// The C example gives the note a function of the flag (lv_obj_bind_bool). A binding
// has no function, so a second state holds the opposite and the note's `hidden`
// follows that.
constinit lv::state<bool> details_wanted{false};
constinit lv::state<bool> note_hidden{true};

constexpr auto box = lv::checkbox({
  .align   = "center",
  .checked = lv::bind(details_wanted),
  .text    = "Show extra details",
});

constexpr auto note = lv::label({
  .align  = "center",
  .hidden = lv::bind(note_hidden),
  .text   = "Extra details only visible while the box is ticked.",
  .y      = 30 * px,
});

}  // namespace

void widgets::checkbox_bind_checked(lv::obj screen) {
  // Once, however often the example is built: the note follows the box from now on.
  static const auto following = details_wanted.observe([](bool wanted) { note_hidden = !wanted; });
  if (following == nullptr) return;  // LVGL had no memory for the observer
  // Both sit on the screen itself: the note is wider than the screen, and the
  // scrollbar this gives must be the screen's own. A tree has one root, so each
  // is a tree of its own.
  lv::place<box>(screen);
  lv::place<note>(screen);
}
