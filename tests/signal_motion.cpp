// lv/signal_motion.hpp: Signal's durations and easings, from include/lv/signal/motion.json.

#include <lv/lv.hpp>
#include <lv/signal_motion.hpp>

#include <type_traits>

namespace {

using namespace lv::units;

// The values of design/signal/tokens.design.json, section "motion".
static_assert(lv::signal::motion.duration.press == 120 * ms);
static_assert(lv::signal::motion.duration.enter == 220 * ms);
static_assert(lv::signal::motion.duration.settle == 480 * ms);
static_assert(*lv::signal::motion.ease.out == lv::bezier{0.2, 0.7, 0.2, 1});
static_assert(*lv::signal::motion.ease.in_out == lv::bezier{0.42, 0, 0.58, 1});

// `back` overshoots: its first y is above 1, which a curve of LVGL takes (y in [-32, 32)).
static_assert(*lv::signal::motion.ease.back == lv::bezier{0.34, 1.56, 0.64, 1});

static_assert(std::is_same_v<decltype(lv::signal::motion.duration.press), lv::token<lv::duration>>);
static_assert(std::is_same_v<decltype(lv::signal::motion.ease.back), lv::token<lv::bezier>>);

// A duration token converts to the plain duration, as every duration token does. The animations
// that take a token are tested with the other tokens (tests/tokens.cpp, tests/animate.cpp).
constexpr lv::duration plain = lv::signal::motion.duration.enter;
static_assert(plain.ms == 220);

}  // namespace

int main() { return 0; }
