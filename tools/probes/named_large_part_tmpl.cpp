// Named pieces, a compile-cost probe (temporary): named_large_part_tmpl
#include <lv/lv.hpp>
using namespace lv::units;

void noop() {}
constexpr auto piece = lv::column({})[
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 0"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})],
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 1"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})],
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 2"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})],
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 3"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})],
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 4"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})],
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 5"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})],
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 6"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})],
    lv::row({.size = {100 * pct, lv::size_content}})[lv::label({.text = "Row 7"}), lv::button({.on_clicked = &noop})[lv::label({.text = "Go"})], lv::slider({.range = {0, 100}})]];
template <const auto& Content>
constexpr auto framed() { return lv::box({.size = {320 * px, 240 * px}})[Content]; }
constexpr auto ui = framed<piece>();

constinit lv::frame<320, 240> frame;

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  return root ? 0 : 1;
}
