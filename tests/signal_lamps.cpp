// lv/signal_lamps.hpp: Signal's tell-tale lamp colours, from include/lv/signal/lamps.json.

#include <lv/lv.hpp>
#include <lv/signal_lamps.hpp>

#include <type_traits>

namespace {

constexpr bool is_rgb(lv_color_t c, int r, int g, int b) { return c.red == r && c.green == g && c.blue == b; }

// The values of design/signal/tokens.design.json, "lamp-green" and "lamp-blue".
static_assert(is_rgb(*lv::signal::lamps.dark.green, 0x3d, 0xdb, 0x84) && is_rgb(*lv::signal::lamps.dark.blue, 0x4d, 0x8d, 0xff));
static_assert(is_rgb(*lv::signal::lamps.day.green, 0x0b, 0x7a, 0x3e) && is_rgb(*lv::signal::lamps.day.blue, 0x1f, 0x3f, 0xbf));
static_assert(std::is_same_v<decltype(lv::signal::lamps.dark.green), lv::token<lv_color_t>>);

// A lamp colour converts to the plain colour, as every colour token does. The styles that take a token
// are tested with the other tokens (tests/tokens.cpp); a tree here would cost 25 G instructions more.
constexpr lv_color_t plain = lv::signal::lamps.day.blue;
static_assert(is_rgb(plain, 0x1f, 0x3f, 0xbf));

}  // namespace

int main() { return 0; }
