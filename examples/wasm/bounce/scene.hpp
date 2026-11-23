#pragma once
// The scene of the site's "Motion, written as a coroutine": taken as it was in examples/window/bounce
// (commit 506e60e^), with the window, the recording and main() left out: the page gives it the display.
//
// A ball that is dropped, bounces lower and lower, and comes to rest, after Qt Quick's
// "Property Animation" example.
//
// The scene is a compile-time tree, styled from design tokens. The motion is coroutines:
//   drop      one fall or rise, eased like gravity (curves from the token file)
//   bounce    the whole sequence: falls and rises that lose height and take less time
//   slide     the sideways motion, so that the ball follows arcs
//   shape     every frame: stretch with speed, squash after a hit, and the shadow
#include <lv/lv.hpp>

#include <cmath>
#include <cstdint>

namespace {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr int screen_width  = 320;
constexpr int screen_height = 480;
constexpr int ball          = 64;   // the ball's diameter at rest
constexpr int box           = 104;  // the box the ball is drawn in: room to squash and stretch
constexpr int ground        = 345;  // the y of the line the ball lands on
constexpr int floor         = ground - box;  // the box's y when the ball is on the ground
constexpr int drop_height   = 250;           // how far the ball falls first
constexpr int margin        = 16;            // room around the shadow for its blurred edge
constexpr int shadow_box    = ball * 11 / 10 + 2 * margin;  // the widget the shadow is blurred in
constexpr int left          = 20;  // the box's x at the left and right ends of its way
constexpr int right         = screen_width - box - 20;

constexpr auto
  scene =
    lv::
      group(
        lv::group({
          .id   = "sky",
          .size = {100 * pct, 50 * pct},
          .style = {
            .bg_color      = tok.sky.top,
            .bg_grad_color = tok.sky.bottom,
            .bg_grad_dir   = LV_GRAD_DIR_VER,  // C API
            .bg_opa        = tok.solid,
          },
        }),
        lv::group({
          .id   = "ground",
          .size = {100 * pct, 50 * pct},
          .y    = 50 * pct,
          .style = {
            .bg_color      = tok.ground.top,
            .bg_grad_color = tok.ground.bottom,
            .bg_grad_dir   = LV_GRAD_DIR_VER,  // C API
            .bg_opa        = tok.solid,
          },
        }),
        // A soft shadow, as under the sun: a dark oval, and over it a clear widget that blurs
        // what is behind it. (LVGL's blur works on a widget's own drawing or on its backdrop,
        // not on its children.)
        lv::group(
          {
            .id   = "shadow",
            .size = {shadow_box * px, (14 + 2 * margin) * px},
            .y    = (ground - 7 - margin) * px,
          },
          lv::group({
            .align = LV_ALIGN_CENTER,  // C API
            .size  = {ball * px, 14 * px},
            .style = {
              .bg_color = tok.shadow.fill,
              .bg_opa   = tok.shadow.strength,
              .radius   = tok.ball.round,
            },
          }),
          lv::group(
            {
              .size = {100 * pct, 100 * pct},
              .style = {
                .blur_backdrop = true,
                .blur_quality  = LV_BLUR_QUALITY_PRECISION,  // C API
                .blur_radius   = tok.shadow.blur,
              },
            }
          )
        ),
        lv::group(
          {
            .id   = "box",
            .size = {box * px, box * px},
            .x    = left * px,
            .y    = (floor - drop_height) * px,
          },
          lv::group({
            .id    = "body",
            .size  = {ball * px, ball * px},
            .style = {
              .bg_color     = tok.ball.fill,
              .bg_opa       = tok.solid,
              .border_color = tok.ball.edge,
              .border_width = tok.ball.edge_width,
              .radius       = tok.ball.round,
            },
          })
        )
      );

// How hard the ball hit the ground last, fading out: 100 right after the first hit.
int squash = 0;

// The time a fall from this height takes. Under gravity it grows with the square root.
lv::duration fall_time(int height) {
  return static_cast<int>(420.0 * std::sqrt(static_cast<double>(height) / drop_height)) * ms;
}

// One fall from `height`, or one rise to it.
lv::task<> fall(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(
    box_view, {.y = {(floor - height) * px, floor * px}}, fall_time(height), *tok.motion.fall
  );
  squash = 100 * height / drop_height;
}
lv::task<> rise(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(
    box_view, {.y = {floor * px, (floor - height) * px}}, fall_time(height), *tok.motion.rise
  );
}

// The time the whole sequence of bounces takes, to move sideways at one speed meanwhile.
lv::duration bounce_time() {
  std::uint32_t total = fall_time(drop_height).ms;
  for (int height = drop_height * 55 / 100; height > 3; height = height * 55 / 100)
    total += 2 * fall_time(height).ms;
  return lv::duration{total};
}

lv::task<> slide(lv::view<"obj"> box_view, int from, int to) {
  co_await lv::animate(box_view, {.x = {from * px, to * px}}, bounce_time());
}

lv::task<> bounce(lv::view<"obj"> box_view) {
  for (bool to_right = true;; to_right = !to_right) {
    lv::spawn(slide(box_view, to_right ? left : right, to_right ? right : left));
    co_await fall(box_view, drop_height);
    // Each bounce reaches 55 percent of the height before it, and so takes less time.
    for (int height = drop_height * 55 / 100; height > 3; height = height * 55 / 100) {
      co_await rise(box_view, height);
      co_await fall(box_view, height);
    }
    co_await lv::sleep(700 * ms);
    co_await rise(box_view, drop_height);  // picked up again
    co_await lv::sleep(250 * ms);
  }
}

// Every frame: the ball's shape from its speed and its last hit, and the shadow under it.
lv::task<> shape(
  lv::view<"obj"> box_view, lv::view<"obj"> body, lv::view<"obj"> shadow, lv::view<"obj"> oval,
  lv::view<"obj"> frost
) {
  int last_y = lv_obj_get_y(box_view.raw());  // C API
  for (;;) {
    const int y     = lv_obj_get_y(box_view.raw());          // C API
    const int x     = lv_obj_get_x(box_view.raw());          // C API
    const int speed = y > last_y ? y - last_y : last_y - y;  // pixels since the last frame
    last_y          = y;

    // Stretched along its way when fast; flattened, and as much wider, after a hit.
    const int stretch = speed > 6 ? 12 : speed * 2;
    const int flat    = ball * squash * 22 / 10000;
    const int height  = ball + stretch - flat;
    const int width   = ball - stretch / 2 + flat;
    body.set({
      .size = {width * px, height * px},
      .x    = (box - width) / 2 * px,
      .y    = (box - height) * px,
    });
    squash = squash * 80 / 100;

    // The shadow under the sun: sharp, dark and as wide as the ball where it touches the
    // ground; wider, lighter and more blurred the higher the ball is. These values are
    // computed every frame, so they are raw values and not design tokens.
    const int above = floor - y;
    const int shade = ball * 3 / 4 + ball * above / drop_height * 2 / 5;
    shadow.set({.x = (x + (box - shadow_box) / 2) * px});
    oval.set(
      {.width = shade * px,
       .style = lv::style({.bg_opa = lv::raw((55 - 30 * above / drop_height) * pct)})}
    );
    frost.set({.style = lv::style({.blur_radius = lv::raw((3 + 9 * above / drop_height) * px)})});
    co_await lv::sleep(10 * ms);
  }
}

void start_scene(lv::obj root) {
  // The tree is mounted: its children are in the order the tree lists them.
  const lv::view<"obj"> shadow{lv_obj_get_child(root.raw(), 2)};    // C API
  const lv::view<"obj"> box_view{lv_obj_get_child(root.raw(), 3)};  // C API
  const lv::view<"obj"> body{lv_obj_get_child(box_view.raw(), 0)};  // C API
  lv::spawn(bounce(box_view));
  const lv::view<"obj"> oval{lv_obj_get_child(shadow.raw(), 0)};   // C API
  const lv::view<"obj"> frost{lv_obj_get_child(shadow.raw(), 1)};  // C API
  lv::spawn(shape(box_view, body, shadow, oval, frost));
}

}  // namespace
