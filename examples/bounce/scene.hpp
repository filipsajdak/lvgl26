#pragma once
// A ball that is dropped, bounces lower and lower, and comes to rest, after Qt Quick's
// "Property Animation" example. The scene of the site's "Motion, written as a coroutine".
//
// The scene knows no platform: a program gives it a screen, and the time comes from its
// lv::application (window.cpp opens a window; tests/example_bounce.cpp steps a clock).
//
// The scene is a compile-time tree, styled from design tokens. The motion is coroutines:
//   fall, rise  one fall or rise, eased like gravity (curves from the token file)
//   bounce      the whole sequence: falls and rises that lose height and take less time
//   slide       the sideways motion, so that the ball follows arcs
//   shape       every frame: stretch with speed, squash after a hit, and the shadow
#include <lv/lv.hpp>

#include <cmath>
#include <cstdint>

namespace bounce {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr int screen_width = 320;
constexpr int screen_height = 480;
constexpr int ball = 64;                 // the ball's diameter at rest
constexpr int box = 104;                 // the box the ball is drawn in: room to squash and stretch
constexpr int ground = 345;              // the y of the line the ball lands on
constexpr int landing = ground - box;      // the box's y when the ball is on the ground
constexpr int drop_height = 250;         // how far the ball falls first
constexpr int margin = 16;               // room around the shadow for its blurred edge
constexpr int shadow_height = 14;        // the shadow's oval at rest
constexpr int shadow_box = ball * 11 / 10 + 2 * margin;  // the widget the shadow is blurred in
constexpr int edge = 20;                 // room between the box and the side of the screen
constexpr int left = edge;               // the box's x at the left and right ends of its way
constexpr int right = screen_width - box - edge;

// How the ball moves.
constexpr int kept_percent = 55;         // each bounce reaches this share of the height before it
constexpr int last_height = 3;           // a bounce lower than this ends the sequence
constexpr int fall_ms = 420;             // a fall from drop_height
constexpr auto rest = 700 * ms;          // on the ground before it is picked up
constexpr auto held = 250 * ms;          // in the air before it is dropped again
constexpr int frame_ms = 10;              // how often the shape is worked out
constexpr auto frame_time = frame_ms * ms;

// How it looks when it moves: in percent of the ball, or of the effect.
constexpr int fast = 6;                  // pixels a frame from which the stretch is full
constexpr int stretch_at_full = 12;      // pixels of stretch at full speed
constexpr int squash_scale = 22;         // how much a full hit flattens the ball, per mille / 10
constexpr int squash_kept = 80;          // what is left of the squash after a frame
constexpr int shade_near = 55;           // the shadow's opacity when the ball is down
constexpr int shade_fade = 30;           // how much lighter it is at the top
constexpr int blur_near = 3;             // the shadow's blur when the ball is down
constexpr int blur_grow = 9;             // how much more it blurs at the top

constexpr auto scene = lv::group()[
    lv::group({.size = {100 * pct, 50 * pct},
               .style = {.bg_color = tok.sky.top, .bg_grad_color = tok.sky.bottom, .bg_grad_dir = "ver",
                         .bg_opa = tok.solid}}),
    lv::group({.size = {100 * pct, 50 * pct},
               .y = 50 * pct,
               .style = {.bg_color = tok.ground.top, .bg_grad_color = tok.ground.bottom, .bg_grad_dir = "ver",
                         .bg_opa = tok.solid}}),
    // A soft shadow, as under the sun: a dark oval, and over it a clear widget that blurs
    // what is behind it. (LVGL's blur works on a widget's own drawing or on its backdrop,
    // not on its children.)
    lv::group({.id = "shadow",
               .size = {shadow_box * px, (shadow_height + 2 * margin) * px},
               .y = (ground - shadow_height / 2 - margin) * px})[
        lv::group({.id = "oval",
                   .align = "center",
                   .size = {ball * px, shadow_height * px},
                   .style = {.bg_color = tok.shadow.fill, .bg_opa = tok.shadow.strength, .radius = tok.ball.round}}),
        lv::group({.id = "frost",
                   .size = {100 * pct, 100 * pct},
                   .style = {.blur_backdrop = true, .blur_quality = "precision", .blur_radius = tok.shadow.blur}})],
    lv::group({.id = "box", .size = {box * px, box * px}, .x = left * px, .y = (landing - drop_height) * px})[
        lv::group({.id = "body",
                   .size = {ball * px, ball * px},
                   .style = {.bg_color = tok.ball.fill, .bg_opa = tok.solid, .border_color = tok.ball.edge,
                             .border_width = tok.ball.edge_width, .radius = tok.ball.round}})]];

// How hard the ball hit the ground last, fading out: 100 right after the first hit.
inline int squash = 0;

// The time a fall from this height takes. Under gravity it grows with the square root.
inline lv::duration fall_time(int height) {
  return static_cast<int>(fall_ms * std::sqrt(static_cast<double>(height) / drop_height)) * ms;
}

constexpr int next_height(int height) { return height * kept_percent / 100; }

//! [fall]
// One fall from `height`, or one rise to it.
inline lv::task<> fall(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(box_view, {.y = {(landing - height) * px, landing * px}}, fall_time(height), *tok.motion.fall);
  squash = 100 * height / drop_height;
}
inline lv::task<> rise(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(box_view, {.y = {landing * px, (landing - height) * px}}, fall_time(height), *tok.motion.rise);
}
//! [fall]

// The time the whole sequence of bounces takes, to move sideways at one speed meanwhile.
inline lv::duration bounce_time() {
  std::uint32_t total = fall_time(drop_height).ms;
  for (int height = next_height(drop_height); height > last_height; height = next_height(height))
    total += 2 * fall_time(height).ms;
  return lv::duration{total};
}

inline lv::task<> slide(lv::view<"obj"> box_view, int from, int to) {
  co_await lv::animate(box_view, {.x = {from * px, to * px}}, bounce_time());
}

//! [bounce]
inline lv::task<> bounce(lv::view<"obj"> box_view) {
  for (bool to_right = true;; to_right = !to_right) {
    lv::spawn(slide(box_view, to_right ? left : right, to_right ? right : left), box_view);
    co_await fall(box_view, drop_height);
    for (int height = next_height(drop_height); height > last_height; height = next_height(height)) {
      co_await rise(box_view, height);
      co_await fall(box_view, height);
    }
    co_await lv::sleep(rest);
    co_await rise(box_view, drop_height);  // picked up again
    co_await lv::sleep(held);
  }
}
//! [bounce]

// The widgets the motion changes.
struct parts {
  lv::view<"obj"> box;
  lv::view<"obj"> body;
  lv::view<"obj"> shadow;
  lv::view<"obj"> oval;
  lv::view<"obj"> frost;
};

// The ball's shape from its speed and its last hit: stretched along its way when fast,
// flattened, and as much wider, after a hit.
inline void shape_ball(lv::view<"obj"> body, int speed) {
  const int stretch = speed > fast ? stretch_at_full : speed * 2;
  const int flat = ball * squash * squash_scale / 10000;
  const int height = ball + stretch - flat;
  const int width = ball - stretch / 2 + flat;
  body.set({.size = {width * px, height * px}, .x = (box - width) / 2 * px, .y = (box - height) * px});
  squash = squash * squash_kept / 100;
}

// The shadow under the sun: sharp, dark and as wide as the ball where it touches the ground;
// wider, lighter and more blurred the higher the ball is. These values are worked out every
// frame, so they are raw values and not design tokens.
inline void shape_shadow(const parts& p, int x, int y) {
  const int above = landing - y;
  const int shade = ball * 3 / 4 + ball * above / drop_height * 2 / 5;
  p.shadow.set({.x = (x + (box - shadow_box) / 2) * px});
  p.oval.set({.width = shade * px,
              .style = lv::style({.bg_opa = lv::raw((shade_near - shade_fade * above / drop_height) * pct)})});
  p.frost.set({.style = lv::style({.blur_radius = lv::raw((blur_near + blur_grow * above / drop_height) * px)})});
}

// The ball's speed in pixels a frame. LVGL moves an animated widget once a display refresh,
// which may be several frames of this task: the speed is the last move spread over the frames
// it took, so the shape does not flicker between fast and still.
struct speedometer {
  int last_y;
  int frames = 0;
  int speed = 0;

  int update(int y) {
    ++frames;
    if (y == last_y) return speed;
    speed = (y > last_y ? y - last_y : last_y - y) / frames;
    last_y = y;
    frames = 0;
    return speed;
  }
};

// Every frame: the ball's shape and the shadow under it.
inline lv::task<> shape(parts p) {
  speedometer speed{.last_y = p.box.get<"y">()};
  for (;;) {
    const int y = p.box.get<"y">();
    shape_ball(p.body, speed.update(y));
    shape_shadow(p, p.box.get<"x">(), y);
    co_await lv::sleep(frame_time);
  }
}

// The scene on a screen: placed in `page`, which owns its widgets, and set moving. False when
// LVGL had no memory for it.
inline bool start(lv::mounted<scene>& page, lv::obj screen) {
  if (!page.mount(screen)) return false;
  const parts p{page->get<"box">(), page->get<"body">(), page->get<"shadow">(), page->get<"oval">(),
                page->get<"frost">()};
  return lv::spawn(bounce(p.box), p.box) && lv::spawn(shape(p), p.box);
}

}  // namespace bounce
