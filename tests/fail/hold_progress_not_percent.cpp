#include <lv/gesture.hpp>
#include <lv/lv.hpp>

// The progress of a hold is a state of percents, not of another type.
lv::task<> wait(lv::obj ring, lv::state<int>& progress) {
  co_await lv::hold(ring, lv::duration{1000}, progress);
}
