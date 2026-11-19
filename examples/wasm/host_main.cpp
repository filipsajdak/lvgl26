// The host build of the demo, driven by a script: a click on each button, a drag of the slider,
// a click on the switch, 1.2 seconds of 16 ms steps. It writes the last frame to standard
// output. The browser runs the same script (page.html) and must produce the same bytes.
#include <cstdint>
#include <cstdio>

extern "C" {
void web_init();
std::uint32_t web_step(std::uint32_t now, std::int32_t x, std::int32_t y, std::int32_t down);
const std::uint8_t* web_frame();
std::int32_t web_width();
std::int32_t web_height();
std::int32_t web_stride();
}

namespace {
struct event {
  std::uint32_t at;
  std::int32_t x;
  std::int32_t y;
  std::int32_t down;
};

// Pointer events, by time. The positions are those of demo::ui at 480 x 360.
#include "script.inc"
}  // namespace

int main() {
  web_init();
  std::int32_t x = 0, y = 0, down = 0;
  std::size_t next = 0;
  for (std::uint32_t t = 0; t <= script_end; t += script_step) {
    while (next < sizeof(script) / sizeof(script[0]) && script[next].at <= t) {
      x = script[next].x;
      y = script[next].y;
      down = script[next].down;
      ++next;
    }
    web_step(t, x, y, down);
  }
  const auto size = static_cast<std::size_t>(web_stride()) * static_cast<std::size_t>(web_height());
  return std::fwrite(web_frame(), 1, size, stdout) == size ? 0 : 1;
}
