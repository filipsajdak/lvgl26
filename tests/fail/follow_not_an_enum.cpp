// A display follows the state of an enum: its values name the themes.
#include <lv/lv.hpp>
#include <lv/signal.hpp>

constinit lv::state<lv::counter> count{lv::counter{}};
constinit lv::frame<64, 48> frame;

void f(lv::application& app) {
  lv::display display{app, frame};
  display.follow(count, {lv::signal::day, lv::signal::dark});
}

int main() {}
