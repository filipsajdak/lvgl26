// The unit types are constexpr, so their tests are static assertions.
#include <lv/lv.hpp>

#include <cstdio>

namespace {

using namespace lv::units;

// The limits of each unit, with the call as the whole initializer: only there does
// GCC 16.2 evaluate a precondition in a constant expression (see lv/detail/checked.hpp).
// These come first, because GCC reuses the result of an earlier call with the same
// arguments.
constexpr auto no_time = 0 * ms;
constexpr auto short_time = 300 * ms;
constexpr auto no_seconds = 0 * sec;
constexpr auto one_second = 1 * sec;
constexpr auto longest = 2'000'000 * sec;
static_assert(no_time.ms == 0 && short_time.ms == 300 && no_seconds.ms == 0);
static_assert(one_second.ms == 1000 && longest.ms == 2'000'000'000u);
constexpr auto widest = 2'147'483'647 * px;
constexpr auto narrowest = (-2'147'483'647 - 1) * px;
static_assert(widest.raw() == 2'147'483'647 && narrowest.raw() == -2'147'483'647 - 1);
constexpr auto black = lv::rgb(0x000000);
constexpr auto white = lv::rgb(0xffffff);
static_assert(black.red == 0 && white.red == 255 && white.blue == 255);

// A unit value that was not given a number is zero.
static_assert(lv::percent{}.value == 0 && lv::length{}.raw() == 0);
static_assert(lv::angle{}.degrees == 0 && lv::angle{}.raw() == 0);
static_assert(lv::duration{}.ms == 0 && lv::duration{}.raw() == 0);
static_assert(lv::counter{}.value == 0);

static_assert((120 * px).raw() == 120 && (0 * px).raw() == 0 && (-4 * px).raw() == -4);
static_assert((50 * pct).value == 50 && lv::length{50 * pct}.raw() == LV_PCT(50));
static_assert(lv::size_content.raw() == LV_SIZE_CONTENT);
static_assert((90 * deg).degrees == 90 && (-90 * deg).raw() == -90);

// A time is not negative, and seconds must fit in milliseconds.
static_assert((0 * ms).ms == 0 && (300 * ms).ms == 300);
static_assert((0 * sec).ms == 0 && (1 * sec).ms == 1000 && (2 * sec).ms == 2000);
static_assert((2'000'000 * sec).ms == 2'000'000'000u);

// An opacity from a share: clamped to 0..100 percent, then scaled to 0..255.
static_assert(lv::opa{}.raw() == LV_OPA_COVER && lv::opa::cover.raw() == 255);
static_assert(lv::opa::transparent.raw() == 0);
static_assert(lv::opa{0 * pct}.raw() == 0 && lv::opa{1 * pct}.raw() == 2);
static_assert(lv::opa{50 * pct}.raw() == 127 && lv::opa{100 * pct}.raw() == 255);
static_assert(lv::opa{-5 * pct}.raw() == 0 && lv::opa{200 * pct}.raw() == 255);
static_assert(lv::opa::from_raw(7).raw() == 7);

// A color from 0xRRGGBB, up to white.
constexpr bool is_rgb(lv_color_t c, int r, int g, int b) { return c.red == r && c.green == g && c.blue == b; }
static_assert(is_rgb(lv::rgb(0x000000), 0, 0, 0) && is_rgb(lv::rgb(0xffffff), 255, 255, 255));
static_assert(is_rgb(lv::rgb(0x3366ff), 0x33, 0x66, 0xff));

// A counter counts.
constexpr int counted() {
  lv::counter c{};
  ++c;
  ++c;
  --c;
  c += 10;
  c -= 4;
  return c.value;
}
static_assert(counted() == 7 && lv::counter{1} < lv::counter{2});

}  // namespace

int main() { std::printf("ok\n"); }
