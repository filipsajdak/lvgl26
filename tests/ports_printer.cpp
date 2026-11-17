// The printer panel (examples/ports/slint/printer): every page is shown and compared with a
// reference picture, and the queue does what its buttons say.
//
//   ports_printer <reference prefix>     e.g. A:tests/ref/printer_ for printer_home.png, ...

#include "check.hpp"

#include <lv/lv.hpp>
#include <scenes.hpp>
#include <shell.hpp>
#include <queue.hpp>

#include <string>

namespace {

constinit lv::frame<printer::window_width, printer::window_height> frame;

// A Start button adds a job to the end of the queue and a Delete button removes it.
void the_queue_follows_its_buttons() {
  const std::size_t before = printer::jobs().size();
  const lv::counter revision = printer::queue_revision.get();
  printer::start_job("report.pdf");
  check(printer::jobs().size() == before + 1);
  check(printer::jobs().back().title == "report.pdf" && printer::jobs().back().progress == 0);
  check(printer::queue_revision.get().value == revision.value + 1);
  printer::cancel_job(before);
  check(printer::jobs().size() == before);
  printer::cancel_job(before + 10);  // there is no such job
  check(printer::jobs().size() == before);
}

// The first job gains a percent each second, and shows it.
void the_print_head_moves(lv::application& app) {
  const int before = printer::jobs().front().progress;
  for (int step = 0; step < 100; ++step) {
    app.advance(10 * lv::ms);
    app.step();
  }
  check(printer::jobs().front().progress == before + 1);
  check(printer::status_line(printer::jobs().front()) == std::to_string(before + 1) + "% - PRINTING");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: ports_printer <reference prefix>\n");
    return 2;
  }
  lv::application app;
  lv::display display{app, frame};
  check(printer::start(display.screen()));
  the_print_head_moves(app);
  the_queue_follows_its_buttons();
  for (const printer::scene& s : printer::scenes) {
    printer::play(app, s);
    const std::string reference = std::string{argv[1]} + s.name + ".png";
    check(lv_test_screenshot_compare(reference.c_str()) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  }
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
