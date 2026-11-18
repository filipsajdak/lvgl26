// The looks example draws what the same screen draws when every design value is written out.

#include "../examples/looks/before.hpp"
#include "same_pixels.hpp"

namespace {

void same_screen() {
  const auto written_out = render([](lv::obj parent) { check(lv::place<looks::before::ui>(parent).raw()); }, nothing);
  const auto with_looks = render([](lv::obj parent) { check(lv::place<looks::ui>(parent).raw()); }, nothing);
  const long at = written_out.first_difference(with_looks);
  check(written_out.valid() && with_looks.valid());
  check(at == -1);
  if (at != -1) written_out.describe_difference(with_looks);
}

}  // namespace

int main() {
  lv::application app;
  constinit static lv::frame<320, 240> frame;
  lv::display display{app, frame};
  same_screen();
  return failures;
}
