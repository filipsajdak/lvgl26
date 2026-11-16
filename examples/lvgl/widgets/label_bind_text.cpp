#include "widgets.hpp"

namespace {

using namespace lv::units;

// What the first label shows. Each button writes its own word into it.
// The C example keeps the text in a string subject with a 256 byte buffer. Here the
// state holds a pointer to a string literal, so nothing is copied.
constinit lv::state<const char*> status{"Hello"};

void show_idle() {
  status = "Idle";
}

void show_running() {
  status = "Running";
}

constexpr auto ui = lv::group[
    lv::label({.align = "center", .text = lv::bind(status), .y = -50 * px}),
    lv::button({.align = "center", .on_clicked = &show_idle})[
        lv::label({.align = "center", .text = "Idle"})],
    lv::button({.align = "center", .on_clicked = &show_running, .y = 50 * px})[
        lv::label({.align = "center", .text = "Running"})]];

}  // namespace

void widgets::label_bind_text(lv::obj screen) {
  lv::place<ui>(screen);
}
