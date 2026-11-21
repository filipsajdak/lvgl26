// Named pieces, a compile-cost probe (temporary): named_small_piece
#include <lv/lv.hpp>
using namespace lv::units;

void noop() {}
constexpr auto piece = lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Name"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})]];
constexpr auto ui = lv::column({})[lv::label({.text = "Title"}), piece, lv::label({.text = "Footer"})];

constinit lv::frame<320, 240> frame;

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  return root ? 0 : 1;
}
