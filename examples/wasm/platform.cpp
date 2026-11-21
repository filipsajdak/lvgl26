// editor-check: skip, it includes app.hpp from the directory of the application it is built with
// What a host needs to drive an application: a clock, a pointer, a frame and one step. The browser
// (page.html) and the host program (host_main.cpp) call the same functions, in the same order.
#include "app.hpp"  // the application: counter/app.hpp or bounce/app.hpp, by the include path

#include <lv/pointer.hpp>

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <new>

void handle_contract_violation(const std::contracts::contract_violation& violation) {
  std::fprintf(stderr, "contract violation: %s\n", violation.comment());
  std::abort();
}

namespace {

constinit std::uint32_t now_ms = 0;

// The union of what changed since the page last drew.
constinit lv::area dirty{};
constinit bool any_dirty = false;
constinit lv::area taken{};

void changed(lv::area a) {
  if (!any_dirty) {
    dirty = a;
    any_dirty = true;
    return;
  }
  const std::int32_t x0 = a.x < dirty.x ? a.x : dirty.x;
  const std::int32_t y0 = a.y < dirty.y ? a.y : dirty.y;
  const std::int32_t x1 = a.x + a.width > dirty.x + dirty.width ? a.x + a.width : dirty.x + dirty.width;
  const std::int32_t y1 = a.y + a.height > dirty.y + dirty.height ? a.y + a.height : dirty.y + dirty.height;
  dirty = {x0, y0, x1 - x0, y1 - y0};
}

lv::duration clock() { return lv::duration{now_ms}; }

constinit lv::frame<app::width, app::height> picture;

struct world {
  lv::application app{{.clock = &clock}};
  lv::display display{app, picture, &changed};
  lv::pointer<> mouse{display};
  decltype(lv::mount<app::ui>(lv::obj{})) root{lv::mount<app::ui>(display.screen())};
  lv::pointer_sample last{};  // what the page said in its last step

  world() {
    // There is no one to report to but the console of the page or the terminal: say it and stop, as before.
    if (!root) {
      std::fputs("the application could not be mounted\n", stderr);
      std::abort();
    }
    app::start(root);
  }

  // A press, a release or a move becomes a sample in the pointer's queue, so a press shorter
  // than LVGL's read period, or than one frame of the page, still reaches it.
  void pointer_at(lv::pointer_sample now) {
    if (now == last) return;
    static_cast<void>(mouse.feed(now.at, now.state));
    last = now;
  }
};

alignas(world) constinit unsigned char world_storage[sizeof(world)];
world* the_world = nullptr;

}  // namespace

extern "C" {

// Starts the application. Call once.
void web_init() { the_world = new (world_storage) world; }

// The pointer, as the page's event says it now is. The page calls it from its event handler,
// between two steps; the sample waits in the queue for the next step.
void web_pointer(std::int32_t x, std::int32_t y, std::int32_t down) {
  the_world->pointer_at({{x, y}, down != 0 ? lv::pressed : lv::released});
}

// The time in milliseconds and the pointer, then one step of LVGL. Returns the milliseconds
// until something is due.
std::uint32_t web_step(std::uint32_t now, std::int32_t x, std::int32_t y, std::int32_t down) {
  now_ms = now;
  the_world->pointer_at({{x, y}, down != 0 ? lv::pressed : lv::released});
  return the_world->app.step().ms;
}

const std::uint8_t* web_frame() { return reinterpret_cast<const std::uint8_t*>(picture.bytes().data()); }
std::int32_t web_stride() { return static_cast<std::int32_t>(picture.stride); }
std::int32_t web_width() { return app::width; }
std::int32_t web_height() { return app::height; }

// Whether something changed since the last call. If so, web_dirty(0..3) is its x, y, width and
// height, until the next call.
std::int32_t web_take_dirty() {
  if (!any_dirty) return 0;
  taken = dirty;
  any_dirty = false;
  return 1;
}

std::int32_t web_dirty(std::int32_t which) {
  const std::int32_t parts[] = {taken.x, taken.y, taken.width, taken.height};
  return parts[which & 3];
}

// The values of the states of the application, for a test of the page.
std::int32_t web_state(std::int32_t which) { return app::state(which); }

// For measurement: the heap that LVGL uses, in bytes.
std::uint32_t web_heap_used() { return static_cast<std::uint32_t>(the_world->app.heap().used); }

}  // extern "C"
