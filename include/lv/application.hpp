#pragma once

// LVGL's lifetime as an object, and the loop that keeps a user interface alive.
//
//   int main() {
//     lv::application app;                     // LVGL starts
//     lv::display display{app, frame};         // see lv/display.hpp
//     auto root = lv::mount<ui>(display.screen());
//     return app.run();                        // timers, animations, input; until app.quit()
//   }                                          // the widgets, the display and LVGL end, in that order
//
// Declare the application first. What is declared after it ends before it.
//
// Time. LVGL has no clock of its own. The application gets the time in one of three ways:
//
//   lv::application app{{.clock = &since_start}};  // a function that returns the time since the start
//   lv::application app{lv::host};                 // the operating system's clock: lv/host.hpp
//   lv::application app;                           // no clock: the program says how much time passed,
//   app.advance(5 * ms);                           //   from a timer interrupt, or step by step in a test
//
// The loop. run() calls step() until quit(). Between two steps it waits for the time until
// something is due: the first event source that can wait does (below), else `idle`, else
// it steps again at once. A program that only draws a picture needs neither.
//
// Event sources. A platform object, such as a window, joins the loop with a member
// lv::in_loop: step() polls every source before LVGL runs (events in: a mouse into its
// pointer), and run() lets the first source that can wait sleep until the next step is due
// or an event comes. A source is declared after its application and ends before it.
//
//   struct my_window {
//     lv::in_loop<my_window> loop;                 // joins app's loop, leaves it when it ends
//     explicit my_window(lv::application& app) : loop{app, *this} {}
//     void poll();                                 // before each step
//     void wait(lv::duration due);                 // optional: until due, or an event
//   };
//
// run() when lvgl26 owns the loop; step() when something else does, such as a browser page
// that calls in once a frame, a test that steps time by hand, or a task of an RTOS. step()
// polls the sources too, so the application's code is the same either way.
//
// There is one application at a time: LVGL's state is global.

#include <lv/units.hpp>

#include <lvgl/lvgl.h>

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace lv {

// LVGL's own heap, in bytes. All zero when LVGL is built to use another allocator.
struct heap_use {
  std::size_t total = 0;         // without the bookkeeping of the blocks, so it varies a little
  std::size_t used = 0;
  std::size_t largest_free = 0;  // the largest block that can still be allocated
};

namespace detail {

// LVGL asks for the time through a function without an argument, so the clock of the
// one application lives here.
inline constinit duration (*application_clock)() = nullptr;

inline std::uint32_t application_time() { return application_clock().ms; }

// One source in the loop of an application: a node of its list, inside the source's object.
struct loop_link {
  loop_link* next = nullptr;
  loop_link* prev = nullptr;
  class application_loop* loop = nullptr;
  void* source = nullptr;
  void (*poll)(void* source) = nullptr;
  void (*wait)(void* source, duration due) = nullptr;  // null for a source that cannot wait
};

// The sources of an application, in the order they joined. Nothing is allocated: each
// node lives in its source.
class application_loop {
  loop_link* first_ = nullptr;
  loop_link* last_ = nullptr;
  loop_link* walking_ = nullptr;  // the node a walk visits next

 public:
  void join(loop_link& n) {
    n.loop = this;
    n.prev = last_;
    n.next = nullptr;
    (last_ != nullptr ? last_->next : first_) = &n;
    last_ = &n;
  }

  // A source that leaves during a walk is not visited by it.
  void leave(loop_link& n) noexcept {
    if (walking_ == &n) walking_ = n.next;
    (n.prev != nullptr ? n.prev->next : first_) = n.next;
    (n.next != nullptr ? n.next->prev : last_) = n.prev;
    n.next = n.prev = nullptr;
    n.loop = nullptr;
  }

  // Polls every source, in the order they joined.
  void poll() {
    walking_ = first_;
    while (walking_ != nullptr) {
      loop_link* n = walking_;
      walking_ = n->next;
      n->poll(n->source);
    }
  }

  // Lets the first source that can wait sleep until `due`, or an event. False if none can.
  bool wait(duration due) const {
    for (loop_link* n = first_; n != nullptr; n = n->next)
      if (n->wait != nullptr) return n->wait(n->source, due), true;
    return false;
  }

  // The application ends: every source leaves, and later ends without touching it.
  void leave_all() noexcept {
    while (first_ != nullptr) leave(*first_);
  }
};

}  // namespace detail

// Something an application's loop drives: it is polled before each step.
template <class S>
concept loop_source = std::is_class_v<S> && requires(S& s) { s.poll(); };

// A source that can also wait: until the next step is due, or an event arrives.
template <class S>
concept waiting_loop_source = loop_source<S> && requires(S& s, duration due) { s.wait(due); };

template <class Source>
class in_loop;

class application {
 public:
  struct options {
    // The time since the program started. Without it, the program calls advance().
    duration (*clock)() = nullptr;
    // What run() does while nothing is due for the given time.
    void (*idle)(duration) = nullptr;
  };

  application() : application(options{}) {}

  explicit application(options how)
    pre(!lv_is_initialized())
      : idle_(how.idle) {
    lv_init();
    detail::application_clock = how.clock;
    if (how.clock != nullptr) lv_tick_set_cb(&detail::application_time);
  }

  application(const application&) = delete;
  application& operator=(const application&) = delete;

  ~application() {
    loop_.leave_all();
    lv_deinit();
    detail::application_clock = nullptr;
  }

  // Tells LVGL that `passed` has gone by. Safe to call from an interrupt.
  // Only for an application without a clock.
  void advance(duration passed) { lv_tick_inc(passed.ms); }

  // Polls every event source, then runs what is due now: timers, animations, input,
  // drawing. Returns the time until something is due again.
  duration step() {
    loop_.poll();
    return duration{lv_timer_handler()};
  }

  // Steps until quit() is called. Between two steps the first source that can wait waits,
  // else `idle`. Returns the result given to quit().
  int run() {
    while (!quit_) {
      const duration due = step();
      if (!quit_) wait(due);
    }
    quit_ = false;
    return result_;
  }

  // How much of LVGL's heap is in use now.
  heap_use heap() const {
    lv_mem_monitor_t now;
    lv_mem_monitor(&now);
    return {now.total_size, now.total_size - now.free_size, now.free_biggest_size};
  }

  // Ends run() after the step that is running. Call it from a handler or a task.
  void quit(int result = 0) {
    result_ = result;
    quit_ = true;
  }

 private:
  template <class Source>
  friend class in_loop;

  void wait(duration due) {
    if (!loop_.wait(due) && idle_ != nullptr) idle_(due);
  }

  detail::application_loop loop_;
  void (*idle_)(duration);
  int result_ = 0;
  bool quit_ = false;
};

// The link that makes `Source` a part of the application's loop: a member of the source,
// constructed with it. It joins in its constructor and leaves in its destructor, so a source
// may end between two steps, or while the loop polls another source. lvgl26 allocates nothing.
// The source is still incomplete where it declares the member, so the constructor checks
// that it is an lv::loop_source.
template <class Source>
class in_loop {
  detail::loop_link link_;

  static void poll(void* s) { static_cast<Source*>(s)->poll(); }
  static void wait(void* s, duration due) { static_cast<Source*>(s)->wait(due); }

 public:
  in_loop(application& app, Source& source)
    requires loop_source<Source>
  {
    link_.source = &source;
    link_.poll = &poll;
    if constexpr (waiting_loop_source<Source>) link_.wait = &wait;
    app.loop_.join(link_);
  }
  in_loop(application&, Source&) = delete(
      "a source of the application's loop has a member function poll(), called before each "
      "step, and may have wait(lv::duration), which waits until the next step is due or an event comes");
  in_loop(application&&, Source&) = delete(
      "a source joins an application that outlives it: declare the application first, then the source");
  in_loop(const in_loop&) = delete;
  in_loop& operator=(const in_loop&) = delete;
  ~in_loop() {
    if (link_.loop != nullptr) link_.loop->leave(link_);
  }

  // Whether the source is in a loop: false once its application has ended.
  bool joined() const noexcept { return link_.loop != nullptr; }
};

}  // namespace lv
