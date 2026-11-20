// What a display costs when nobody sees it: 24 animated arcs on a 480 by 320 display, for some
// simulated seconds, against LVGL's C API with no lvgl26 (docs/research/visibility.md).
//
//   visibility_probe SCENARIO SECONDS
//
// SCENARIO is visible (the arcs are drawn), hidden (each arc has the hidden flag) or
// invalidation_off (LVGL is told not to mark anything for redraw). It prints one JSON object with
// the flushes, the pixels and the animation calls. The nightly run counts the instructions of the
// whole process with Valgrind, for two durations, and takes the difference (tools/compile_cost.py).
#include <lvgl/lvgl.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr std::int32_t display_width = 480;
constexpr std::int32_t display_height = 320;
constexpr std::int32_t band_height = 40;
constexpr std::int32_t arc_count = 24;
constexpr std::int32_t arc_side = 70;
constexpr std::int32_t arcs_per_row = display_width / arc_side;
constexpr std::uint32_t step_ms = 5;
constexpr std::uint32_t ms_per_second = 1000;
constexpr std::uint32_t animation_period_ms = 1000;
constexpr std::int32_t arc_full = 100;

std::uint64_t flushes = 0;
std::uint64_t pixels = 0;
std::uint64_t animation_calls = 0;

constinit std::uint16_t band[display_width * band_height];

void flush(lv_display_t* display, const lv_area_t* area, std::uint8_t*) {
  ++flushes;
  pixels += static_cast<std::uint64_t>(lv_area_get_width(area)) * static_cast<std::uint64_t>(lv_area_get_height(area));
  lv_display_flush_ready(display);
}

void move_arc(void* arc, std::int32_t value) {
  ++animation_calls;
  lv_arc_set_value(static_cast<lv_obj_t*>(arc), value);
}

void add_arc(std::int32_t index, bool hidden) {
  lv_obj_t* arc = lv_arc_create(lv_screen_active());
  lv_obj_set_size(arc, arc_side, arc_side);
  lv_obj_set_pos(arc, (index % arcs_per_row) * arc_side, (index / arcs_per_row) * arc_side);
  lv_anim_t animation;
  lv_anim_init(&animation);
  lv_anim_set_var(&animation, arc);
  lv_anim_set_exec_cb(&animation, move_arc);
  lv_anim_set_values(&animation, 0, arc_full);
  lv_anim_set_duration(&animation, animation_period_ms);
  lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&animation);
  if (hidden) lv_obj_set_hidden(arc, true);
}

void run(std::uint32_t seconds) {
  for (std::uint32_t elapsed = 0; elapsed < seconds * ms_per_second; elapsed += step_ms) {
    lv_tick_inc(step_ms);
    lv_timer_handler();
  }
}

bool known(const char* scenario) {
  return std::strcmp(scenario, "visible") == 0 || std::strcmp(scenario, "hidden") == 0 ||
         std::strcmp(scenario, "invalidation_off") == 0;
}

}  // namespace

int main(int argc, char** argv) {
  constexpr int expected_arguments = 3;
  if (argc != expected_arguments || !known(argv[1])) {
    std::fputs("usage: visibility_probe visible|hidden|invalidation_off SECONDS\n", stderr);
    return 2;
  }
  const char* scenario = argv[1];
  const auto seconds = static_cast<std::uint32_t>(std::strtoul(argv[2], nullptr, 10));

  lv_init();
  lv_display_t* display = lv_display_create(display_width, display_height);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(display, flush);
  lv_display_set_buffers(display, band, nullptr, sizeof(band), LV_DISPLAY_RENDER_MODE_PARTIAL);
  for (std::int32_t i = 0; i < arc_count; ++i) add_arc(i, std::strcmp(scenario, "hidden") == 0);
  lv_timer_handler();  // the first draw is not part of a second
  if (std::strcmp(scenario, "invalidation_off") == 0) lv_display_enable_invalidation(display, false);
  flushes = pixels = animation_calls = 0;

  run(seconds);
  std::printf("{\"scenario\":\"%s\",\"seconds\":%u,\"flushes\":%llu,\"pixels\":%llu,\"animation_calls\":%llu}\n",
              scenario, seconds, static_cast<unsigned long long>(flushes), static_cast<unsigned long long>(pixels),
              static_cast<unsigned long long>(animation_calls));
  lv_deinit();
  return 0;
}
