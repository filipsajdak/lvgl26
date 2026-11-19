// The motion of examples/bounce/scene.hpp, on a clock that steps 10 ms at a time: the ball falls
// in the time the scene says, lands on the floor, bounces lower each time, moves sideways the
// whole way, and squashes and stretches. The pictures at fixed times are compared by the test
// example_bounce_pictures, on LVGL v9.6.0 only.

#include "check.hpp"

#include <scene.hpp>

#include <algorithm>
#include <cstdio>

namespace {

using namespace bounce;

// LVGL moves an animated widget once a display refresh: every 33 ms by default, so an
// animation's end is seen up to that much later than the clock says it ends.
constexpr int refresh_lag_ms = 50;

constinit lv::frame<screen_width, screen_height> frame;
constinit lv::mounted<scene> page;

// The box's place and the ball's size at one step.
struct sample {
  int at;
  int x;
  int y;
  int width;
  int height;
};

// One step of the clock: frame_time, the scene's own.
sample step(lv::application& app, int at) {
  app.advance(frame_time);
  app.step();
  const auto box_view = page->get<"box">();
  const auto body = page->get<"body">();
  return {at, box_view.get<"x">(), box_view.get<"y">(), body.get<"width">(), body.get<"height">()};
}

// What the steps of the first full sequence of bounces showed.
struct seen {
  int first_landing = -1;  // when the box first reached the floor
  int first_peak = landing;  // the highest point after the first landing
  int lowest_after = 0;    // the box's lowest y: the floor, never below it
  int widest = 0;          // the ball's widest: a squash
  int tallest = 0;         // the ball's tallest: a stretch
  int left_most = right;
  int right_most = left;
};

void take(seen& s, const sample& now) {
  if (s.first_landing < 0 && now.y == landing) s.first_landing = now.at;
  if (s.first_landing >= 0) s.first_peak = std::min(s.first_peak, now.y);
  s.lowest_after = std::max(s.lowest_after, now.y);
  s.widest = std::max(s.widest, now.width);
  s.tallest = std::max(s.tallest, now.height);
  s.left_most = std::min(s.left_most, now.x);
  s.right_most = std::max(s.right_most, now.x);
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  check(start(page, display.screen()));
  if (!page) return failures;

  // The box starts drop_height above the floor, at the left.
  const sample first = step(app, frame_ms);
  check(first.y == landing - drop_height && first.x == left);

  seen s;
  const int sequence = static_cast<int>(bounce_time().ms);  // a few seconds: fits an int
  for (int at = 2 * frame_ms; at <= sequence + refresh_lag_ms; at += frame_ms) take(s, step(app, at));
  const int fall = static_cast<int>(fall_time(drop_height).ms);
  std::printf("landed at %d ms (a fall takes %d), first peak %d, widest %d, tallest %d, x %d..%d\n",
              s.first_landing, fall, s.first_peak, s.widest, s.tallest, s.left_most, s.right_most);

  // The first fall ends on the floor in the time the scene gives it.
  check(s.first_landing >= fall && s.first_landing <= fall + refresh_lag_ms);
  check(s.lowest_after == landing);
  // The first bounce reaches kept_percent of the drop, within a pixel.
  const int peak = landing - next_height(drop_height);
  check(s.first_peak >= peak - 1 && s.first_peak <= peak + 1);
  // A hit flattens the ball and speed stretches it.
  check(s.widest > ball && s.tallest > ball);
  // Meanwhile it moved all the way from the left to the right.
  check(s.left_most == left && s.right_most == right);
  return failures;
}
