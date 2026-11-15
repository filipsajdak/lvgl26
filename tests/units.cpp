// The unit types are constexpr, so most of their tests are static assertions. The
// angles with tenths of a degree are also tested at run time, with values the
// compiler cannot fold.
#include <lv/lv.hpp>

#include "check.hpp"

#include <concepts>
#include <cstdio>
#include <cstring>
#include <limits>

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
// The largest and smallest angles in tenths of a degree: from whole degrees, from
// degrees with a fraction, and from a number.
constexpr lv::decidegrees most_turns = 214'748'364 * deg;
constexpr lv::decidegrees fewest_turns = -214'748'364 * deg;
constexpr auto finest = 214'748'364.7 * deg;
constexpr auto lowest = -214'748'364.8 * deg;
constexpr auto from_most = lv::decidegrees::from_number(214'748'364.7);
constexpr auto from_least = lv::decidegrees::from_number(-214'748'364.8);
static_assert(most_turns.tenths == 2'147'483'640 && fewest_turns.tenths == -2'147'483'640);
static_assert(finest.tenths == 2'147'483'647 && lowest.tenths == -2'147'483'647 - 1);
static_assert(from_most == finest && from_least == lowest);
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

// A whole angle is an lv::angle. With a fraction it is lv::decidegrees, in tenths.
static_assert(std::same_as<decltype(90 * deg), lv::angle>);
static_assert(std::same_as<decltype(22.5 * deg), lv::decidegrees>);
static_assert(lv::decidegrees{}.tenths == 0 && lv::decidegrees{}.raw() == 0);
static_assert((22.5 * deg).tenths == 225 && (22.5 * deg).raw() == 225);
static_assert((0.1 * deg).tenths == 1 && (-0.1 * deg).tenths == -1 && (0.0 * deg).tenths == 0);
static_assert((22.3 * deg).tenths == 223 && (-22.3 * deg).tenths == -223);  // not exact in a double
static_assert((90.0 * deg).tenths == 900 && (-0.7 * deg).tenths == -7);
static_assert(lv::decidegrees::from_number(22.5).tenths == 225);
static_assert(lv::decidegrees::from_number(-360).tenths == -3600);

// A whole angle goes where tenths are taken; tenths do not go where whole degrees are.
static_assert(std::convertible_to<lv::angle, lv::decidegrees>);
static_assert(!std::convertible_to<lv::decidegrees, lv::angle>);
static_assert(!std::constructible_from<lv::angle, lv::decidegrees>);
static_assert(lv::decidegrees(90 * deg).tenths == 900 && lv::decidegrees(-1 * deg).tenths == -10);
static_assert(lv::decidegrees(0 * deg).tenths == 0);
// Like lv::angle{90}, braces give the member itself: tenths.
static_assert(lv::decidegrees{900} == 90 * deg);

// The two kinds compare by the angle they stand for, in either order.
static_assert(90.0 * deg == 90 * deg && 90 * deg == 90.0 * deg);
static_assert(22.5 * deg != 22 * deg && 22 * deg != 22.5 * deg && 9 * deg != 90.0 * deg);
static_assert(22.5 * deg == 22.5 * deg && 22.5 * deg != 22.6 * deg);
// 214748365 degrees are more tenths than an int32_t holds: no fine angle equals them.
static_assert(lv::decidegrees{-2'147'483'646} != 214'748'365 * deg);

// Only a `double` is degrees with a fraction: a float or a bool is not a number of degrees.
template <class T>
concept times_deg = requires(T n) { n * deg; };
static_assert(times_deg<double> && times_deg<int> && !times_deg<float> && !times_deg<long double>);
static_assert(!times_deg<bool> && !times_deg<char>);

// Which whole degrees fit as tenths.
static_assert(lv::detail::fits_tenths(0) && lv::detail::fits_tenths(214'748'364));
static_assert(lv::detail::fits_tenths(-214'748'364));
static_assert(!lv::detail::fits_tenths(214'748'365) && !lv::detail::fits_tenths(-214'748'365));

// Which degrees are a whole number of tenths: within a thousandth of a tenth.
static_assert(lv::detail::whole_tenths(0) && lv::detail::whole_tenths(22.5));
static_assert(lv::detail::whole_tenths(-22.5) && lv::detail::whole_tenths(22.3));
static_assert(!lv::detail::whole_tenths(22.55) && !lv::detail::whole_tenths(-22.55));
static_assert(!lv::detail::whole_tenths(0.05) && !lv::detail::whole_tenths(-0.05));
static_assert(lv::detail::whole_tenths(22.50009) && !lv::detail::whole_tenths(22.5001));
static_assert(lv::detail::whole_tenths(-22.50009) && !lv::detail::whole_tenths(-22.5001));
// Exactly a thousandth of a tenth off is already too far, on either side.
static_assert(!lv::detail::whole_tenths(0.0001) && !lv::detail::whole_tenths(-0.0001));
static_assert(0.0001 * 10 == 0.001);  // what makes the line above a test of the limit itself
static_assert(lv::detail::whole_tenths(214'748'364.7) && !lv::detail::whole_tenths(214'748'364.8));
static_assert(lv::detail::whole_tenths(-214'748'364.8) && !lv::detail::whole_tenths(-214'748'364.9));
static_assert(!lv::detail::whole_tenths(1e300) && !lv::detail::whole_tenths(-1e300));
static_assert(!lv::detail::whole_tenths(std::numeric_limits<double>::quiet_NaN()));
static_assert(!lv::detail::whole_tenths(std::numeric_limits<double>::infinity()));
static_assert(lv::detail::nearest_tenth(22.54) == 225 && lv::detail::nearest_tenth(22.56) == 226);
static_assert(lv::detail::nearest_tenth(-22.54) == -225 && lv::detail::nearest_tenth(-22.56) == -226);
static_assert(lv::detail::nearest_tenth(0) == 0);

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

// The same at run time. The numbers come from the command line or through a
// volatile, so the compiler cannot work the result out.
void decidegreess_at_run_time() {
  volatile double fraction = 22.5;
  volatile std::int32_t whole = 90;
  const double f = fraction;
  const std::int32_t w = whole;

  check((f * deg).tenths == 225 && (-f * deg).tenths == -225);
  check(lv::decidegrees::from_number(f).tenths == 225);
  check(lv::decidegrees::from_number(-f - 0.2).tenths == -227);
  const lv::decidegrees from_whole = w * deg;
  check(from_whole.tenths == 900 && from_whole == w * deg && from_whole != (w + 1) * deg);
  const lv::decidegrees negative = -w * deg;
  check(negative.tenths == -900);

  check(lv::detail::whole_tenths(f) && !lv::detail::whole_tenths(f + 0.05));
  check(!lv::detail::whole_tenths(f * 1e9) && !lv::detail::whole_tenths(-f * 1e9));
  check(lv::detail::fits_tenths(w) && !lv::detail::fits_tenths(w * 10'000'000));
  check(!lv::detail::fits_tenths(-w * 10'000'000));
}

// A value that breaks a contract at run time ends the program with a contract
// violation. Each case is run as a test of its own, which expects that.
int violates(const char* which) {
  volatile double not_a_tenth = 22.55;
  volatile std::int32_t too_many_degrees = 214'748'365;
  if (std::strcmp(which, "fraction") == 0) return (not_a_tenth * deg).tenths;
  if (std::strcmp(which, "number") == 0) return lv::decidegrees::from_number(not_a_tenth).tenths;
  if (std::strcmp(which, "whole") == 0) return lv::decidegrees(too_many_degrees * deg).tenths;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    std::printf("no contract violation: %d\n", violates(argv[1]));
    return 0;
  }
  decidegreess_at_run_time();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
