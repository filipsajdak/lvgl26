// A ball that is dropped, bounces lower and lower, and comes to rest, after Qt Quick's
// "Property Animation" example.
//
//   bounce                 opens a window
//   bounce --record <dir>  writes six seconds of frames to <dir> as frame_0000.ppm, ...
//
// The scene is a compile-time tree, styled from design tokens. The motion is coroutines:
//   drop      one fall or rise, eased like gravity (curves from the token file)
//   bounce    the whole sequence: falls and rises that lose height and take less time
//   slide     the sideways motion, so that the ball follows arcs
//   shape     every frame: stretch with speed, squash after a hit, and the shadow
#include <lv/lv.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr int screen_width = 320;
constexpr int screen_height = 480;
constexpr int ball = 64;         // the ball's diameter at rest
constexpr int box = 104;         // the box the ball is drawn in: room to squash and stretch
constexpr int ground = 345;      // the y of the line the ball lands on
constexpr int floor = ground - box;  // the box's y when the ball is on the ground
constexpr int drop_height = 250; // how far the ball falls first
constexpr int margin = 16;      // room around the shadow for its blurred edge
constexpr int shadow_box = ball * 11 / 10 + 2 * margin;  // the widget the shadow is blurred in
constexpr int left = 20;         // the box's x at the left and right ends of its way
constexpr int right = screen_width - box - 20;

constexpr auto scene = lv::group()[
    lv::group({.id = "sky",
               .size = {100 * pct, 50 * pct},
               .style = {.bg_color = tok.sky.top, .bg_grad_color = tok.sky.bottom,
                         .bg_grad_dir = LV_GRAD_DIR_VER, .bg_opa = tok.solid}}),
    lv::group({.id = "ground",
               .size = {100 * pct, 50 * pct},
               .style = {.bg_color = tok.ground.top, .bg_grad_color = tok.ground.bottom,
                         .bg_grad_dir = LV_GRAD_DIR_VER, .bg_opa = tok.solid},
               .y = 50 * pct}),
    // A soft shadow, as under the sun: a dark oval, and over it a clear widget that blurs
    // what is behind it. (LVGL's blur works on a widget's own drawing or on its backdrop,
    // not on its children.)
    lv::group({.id = "shadow", .size = {shadow_box * px, (14 + 2 * margin) * px}, .y = (ground - 7 - margin) * px})[
        lv::group({.align = LV_ALIGN_CENTER,
                   .size = {ball * px, 14 * px},
                   .style = {.bg_color = tok.shadow.fill, .bg_opa = tok.shadow.strength,
                             .radius = tok.ball.round}}),
        lv::group({.size = {100 * pct, 100 * pct},
                   .style = {.blur_backdrop = true, .blur_quality = LV_BLUR_QUALITY_PRECISION,
                             .blur_radius = tok.shadow.blur}})],
    lv::group({.id = "box", .size = {box * px, box * px}, .x = left * px, .y = (floor - drop_height) * px})[
        lv::group({.id = "body",
                   .size = {ball * px, ball * px},
                   .style = {.bg_color = tok.ball.fill, .bg_opa = tok.solid,
                             .border_color = tok.ball.edge, .border_width = tok.ball.edge_width,
                             .radius = tok.ball.round}})]];

// How hard the ball hit the ground last, fading out: 100 right after the first hit.
int squash = 0;

// The time a fall from this height takes. Under gravity it grows with the square root.
lv::duration fall_time(int height) {
  return static_cast<int>(420.0 * std::sqrt(static_cast<double>(height) / drop_height)) * ms;
}

// One fall from `height`, or one rise to it.
lv::task<> fall(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(box_view, {.y = {(floor - height) * px, floor * px}}, fall_time(height),
                       *tok.motion.fall);
  squash = 100 * height / drop_height;
}
lv::task<> rise(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(box_view, {.y = {floor * px, (floor - height) * px}}, fall_time(height),
                       *tok.motion.rise);
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
lv::task<> shape(lv::view<"obj"> box_view, lv::view<"obj"> body, lv::view<"obj"> shadow,
                 lv::view<"obj"> oval, lv::view<"obj"> frost) {
  int last_y = lv_obj_get_y(box_view.raw());
  for (;;) {
    const int y = lv_obj_get_y(box_view.raw());
    const int x = lv_obj_get_x(box_view.raw());
    const int speed = y > last_y ? y - last_y : last_y - y;  // pixels since the last frame
    last_y = y;

    // Stretched along its way when fast; flattened, and as much wider, after a hit.
    const int stretch = speed > 6 ? 12 : speed * 2;
    const int flat = ball * squash * 22 / 10000;
    const int height = ball + stretch - flat;
    const int width = ball - stretch / 2 + flat;
    body.set({.size = {width * px, height * px}, .x = (box - width) / 2 * px, .y = (box - height) * px});
    squash = squash * 80 / 100;

    // The shadow under the sun: sharp, dark and as wide as the ball where it touches the
    // ground; wider, lighter and more blurred the higher the ball is. These values are
    // computed every frame, so they are raw values and not design tokens.
    const int above = floor - y;
    const int shade = ball * 3 / 4 + ball * above / drop_height * 2 / 5;
    shadow.set({.x = (x + (box - shadow_box) / 2) * px});
    oval.set({.style = {.bg_opa = lv::raw((55 - 30 * above / drop_height) * pct)}, .width = shade * px});
    frost.set({.style = {.blur_radius = lv::raw((3 + 9 * above / drop_height) * px)}});
    co_await lv::sleep(10 * ms);
  }
}

void start(lv::obj screen) {
  const lv::obj root = lv::place<scene>(screen);
  // The tree is placed, so its widgets live as long as the screen. Its children are
  // in the order the tree lists them.
  const lv::view<"obj"> shadow{lv_obj_get_child(root.raw(), 2)};
  const lv::view<"obj"> box_view{lv_obj_get_child(root.raw(), 3)};
  const lv::view<"obj"> body{lv_obj_get_child(box_view.raw(), 0)};
  lv::spawn(bounce(box_view));
  const lv::view<"obj"> oval{lv_obj_get_child(shadow.raw(), 0)};
  const lv::view<"obj"> frost{lv_obj_get_child(shadow.raw(), 1)};
  lv::spawn(shape(box_view, body, shadow, oval, frost));
}

// ---- recording --------------------------------------------------------------

void flush(lv_display_t* display, const lv_area_t*, std::uint8_t*) { lv_display_flush_ready(display); }

bool write_frame(const std::string& directory, int number) {
  lv_draw_buf_t* shot = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
  if (shot == nullptr) return false;
  char name[32];
  std::snprintf(name, sizeof name, "/frame_%04d.ppm", number);
  std::FILE* file = std::fopen((directory + name).c_str(), "wb");
  if (file != nullptr) {
    std::fprintf(file, "P6\n%d %d\n255\n", static_cast<int>(shot->header.w), static_cast<int>(shot->header.h));
    for (std::uint32_t y = 0; y < shot->header.h; ++y) {
      const std::uint8_t* row = shot->data + y * shot->header.stride;
      for (std::uint32_t x = 0; x < shot->header.w; ++x) {
        const std::uint8_t rgb[3] = {row[x * 4 + 2], row[x * 4 + 1], row[x * 4]};
        std::fwrite(rgb, 1, 3, file);
      }
    }
    std::fclose(file);
  }
  lv_draw_buf_destroy(shot);
  return file != nullptr;
}

int record(const std::string& directory) {
  static std::vector<std::uint8_t> pixels(screen_width * screen_height * 4);
  lv_display_t* display = lv_display_create(screen_width, screen_height);
  lv_display_set_buffers(display, pixels.data(), nullptr, static_cast<std::uint32_t>(pixels.size()),
                         LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(display, &flush);
  start(lv::screen_active());
  constexpr int frames = 6 * 50;  // six seconds at 50 frames a second
  for (int frame = 0; frame < frames; ++frame) {
    for (int step = 0; step < 2; ++step) {  // LVGL steps every 10 ms
      lv_tick_inc(10);
      lv_timer_handler();
    }
    if (!write_frame(directory, frame)) return 1;
  }
  std::printf("wrote %d frames to %s\n", frames, directory.c_str());
  return 0;
}

int show() {
  lv_sdl_window_create(screen_width, screen_height);
  lv_sdl_mouse_create();
  start(lv::screen_active());
  for (;;) {
    // Sleep until LVGL's next timer is due, and never long enough to miss a frame.
    const std::uint32_t wait = lv_timer_handler();
    std::this_thread::sleep_for(std::chrono::milliseconds(wait < 1 ? 1 : wait > 4 ? 4 : wait));
  }
}

}  // namespace

int main(int argc, char** argv) {
  lv_init();
  if (argc == 3 && std::string_view(argv[1]) == "--record") return record(argv[2]);
  return show();
}
