// lv/gesture.hpp, what a program cannot reach through the public names: the handler a wait takes, a wait
// without memory for its tie to the widget, and the preconditions of arm(). Run with a case name, it breaks one.
#include <lv/gesture.hpp>
#include <lv/lv.hpp>

#include "check.hpp"
#include "full_heap.hpp"

#include <cstdio>
#include <string_view>
#include <utility>

namespace {

using namespace lv::units;

constinit lv::frame<64, 64> frame;
lv::state<lv::percent> progress{lv::percent{0}};

// A handler of a swipe takes the direction or nothing; of a hold, whether it lasted or nothing.
template <class Source, class F>
concept takes_handler = requires(Source s, F f) { std::move(s).then(f); };
static_assert(takes_handler<lv::detail::swipe_source<>, void (*)(lv::swipe)> && takes_handler<lv::detail::swipe_source<>, void (*)()>);
static_assert(!takes_handler<lv::detail::swipe_source<>, void (*)(int, int)>);
static_assert(takes_handler<lv::detail::hold_source, void (*)(bool)> && takes_handler<lv::detail::hold_source, void (*)()>);
static_assert(!takes_handler<lv::detail::hold_source, void (*)(int, int)>);

// Sets a flag when the task that holds it ends.
struct at_exit {
  bool* flag;
  ~at_exit() { *flag = true; }
};
lv::task<> wait_and_mark(lv::obj widget, bool* ended) {
  at_exit mark{ended};
  co_await lv::swiped(widget);
}
lv::task<> hold_and_mark(lv::obj widget, bool* ended) {
  at_exit mark{ended};
  co_await lv::hold(widget, 1000 * ms, progress);
}

// Without memory to tie the task to the widget, the wait ends the task at once and touches nothing: the
// widget still passes gestures on, and the progress of a hold under a finger that is down already stays.
void a_wait_without_memory_ends_the_task() {
  auto widget = lv::screen_active().add<"obj">();
  bool swipe_ended = false;
  bool hold_ended = false;
  {
    auto work = wait_and_mark(widget, &swipe_ended);  // the frame is made before the heap is full
    full_heap heap{sizeof(lv::detail::widget_watch)};  // the record fits, its LVGL handler not
    check(lv::spawn(std::move(work)));
  }
  progress.set(lv::percent{50});
  lv_obj_add_state(widget.raw(), LV_STATE_PRESSED);  // a finger down: the hold would start at once
  {
    auto work = hold_and_mark(widget, &hold_ended);
    full_heap heap{sizeof(lv::detail::widget_watch)};
    check(lv::spawn(std::move(work)));
  }
  check(swipe_ended && hold_ended && lv_obj_is_gesture_bubble(widget.raw()) && progress.get().value == 50);
  lv_obj_delete(widget.raw());
}

// Waits on a widget that is none: the precondition of the wait ends the program.
lv::task<> swipe_of_nothing() { co_await lv::swiped(lv::obj{nullptr}); }
lv::task<> hold_of_nothing() { co_await lv::hold(lv::obj{nullptr}, 1000 * ms); }

void no_finish(void*) {}

// Each breaks a precondition of a wait.
int broken(std::string_view which) {
  if (which == "swipe_of_nothing") check(lv::spawn(swipe_of_nothing()));
  if (which == "hold_of_nothing") check(lv::spawn(hold_of_nothing()));
  auto widget = lv::screen_active().add<"obj">();
  if (which == "swipe_without_task") lv::detail::swipe_source<>{widget.raw()}.arm(&no_finish, nullptr, nullptr);
  if (which == "hold_without_task") lv::detail::hold_source{widget.raw(), 1000, nullptr}.arm(&no_finish, nullptr, nullptr);
  std::printf("no contract violation\n");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  lv::application app;
  lv::display display{app, frame};
  if (argc == 2) return broken(argv[1]);
  a_wait_without_memory_ends_the_task();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
