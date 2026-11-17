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
  const std::size_t before = printer::queue.size();
  const printer::job_id id = printer::start_job("report.pdf");
  check(id.value != 0 && printer::queue.size() == before + 1);
  check(std::string{printer::queue[before].title} == "report.pdf" && printer::queue[before].progress.value == 0);
  printer::cancel_job(id);
  check(printer::queue.size() == before);
  printer::cancel_job(printer::job_id{0});  // there is no such job
  check(printer::queue.size() == before);
}

// The first job gains a percent each second, and shows it.
void the_print_head_moves(lv::application& app) {
  const int before = printer::queue[0].progress.value;
  for (int step = 0; step < 100; ++step) {
    app.advance(10 * lv::ms);
    app.step();
  }
  check(printer::queue[0].progress.value == before + 1);
  check(std::string{printer::queue[0].status} == std::to_string(before + 1) + "% - PRINTING");
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
