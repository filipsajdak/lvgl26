// The slice screen drawn into a whole frame, written to standard output as raw bytes.
// The same file is built for the host and for WebAssembly; the two outputs must be equal.
#include <lv/lv.hpp>

#include <contracts>
#include <cstdio>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation& violation) {
  std::fprintf(stderr, "contract violation: %s\n", violation.comment());
  std::abort();
}

namespace {

using namespace lv::units;

constexpr auto ui = lv::column(
    {.flex_align = {LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER},
     .size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .size = {110 * px, 110 * px}, .value = 120}),
    lv::button({.id = "reset"})[lv::label({.text = "Reset"})]];

constinit lv::frame<320, 240> picture;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, picture};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  for (int passed = 0; passed < 100; passed += 5) {
    app.advance(5 * ms);
    app.step();
  }
  const auto bytes = picture.bytes();
  return std::fwrite(bytes.data(), 1, bytes.size(), stdout) == bytes.size() ? 0 : 1;
}
