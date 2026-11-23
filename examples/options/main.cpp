// Option lists made from an enum: the texts of the options, and some enumerators left out.
// docs/options.md walks through this file.

#include <lv/lv.hpp>

using namespace lv::units;

namespace {

enum class sheet { a4, letter, legal, a5 };

}  // namespace

//! [texts]
// The texts of an enum's options, given instead of the names of its enumerators.
template <>
struct lv::option_text<sheet> {
  static consteval const char* of(sheet s) {
    switch (s) {
      case sheet::a4:
        return "A4";
      case sheet::letter:
        return "Letter";
      case sheet::legal:
        return "Legal";
      case sheet::a5:
        return "A5";
    }
    return "";
  }
};
//! [texts]

namespace {

//! [but]
// The sheets of this printer's tray: A5 is left out, so the list has three options.
constexpr auto tray = lv::options_of<sheet, lv::but<sheet::a5>>;
//! [but]

constinit lv::state<sheet> paper{sheet::letter};

constexpr auto ui = lv::dropdown({
  .id       = "paper",
  .options  = tray,
  .selected = lv::bind(paper),
  .width    = 120 * px,
});

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  auto box = root->get<"paper">();
  if (box.get<"selected">() != 1) return 2;  // Letter
  paper = sheet::legal;
  if (box.get<"selected">() != 2) return 3;
  return 0;
}
