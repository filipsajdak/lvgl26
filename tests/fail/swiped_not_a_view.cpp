#include <lv/gesture.hpp>
#include <lv/lv.hpp>

// A swipe is waited for on a view of a widget, not on a number.
lv::task<> wait() {
  co_await lv::swiped(3);
}
