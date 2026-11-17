#include "queue.hpp"

#include <algorithm>

namespace printer {
namespace {

using namespace lv::units;

constexpr int percent_done = 100;

std::vector<job>& model() {
  static std::vector<job> jobs{
      {.title = "210106-FinalPresentation.pdf", .owner = "info@example.com", .size = "143kb", .pages = 6,
       .progress = 63, .printing = true},
      {.title = "Adressliste.docx", .owner = "info@example.com", .size = "143kb", .pages = 6},
      {.title = "Salaries.pdf", .owner = "info@example.com", .size = "143kb", .pages = 6},
      {.title = "EW 2026 Slides", .owner = "info@example.com", .size = "2.5MB", .submitted = "11:41 10/03/26",
       .pages = 300},
  };
  return jobs;
}

void changed() { queue_revision.update([](lv::counter n) { return lv::counter{n.value + 1}; }); }

// One second of printing: the first job gains a percent.
void print_a_second() {
  auto& jobs = model();
  if (jobs.empty()) return;
  job& first = jobs.front();
  first.progress += 1;
  first.printing = true;
  if (first.progress > percent_done) jobs.erase(jobs.begin());
  changed();
}

}  // namespace

const std::vector<job>& jobs() { return model(); }

void start_job(const char* title) {
  model().push_back({.title = title});
  changed();
}

void cancel_job(std::size_t index) {
  if (index >= model().size()) return;
  model().erase(model().begin() + static_cast<std::ptrdiff_t>(index));
  changed();
}

lv::task<> run_queue() {
  for (;;) {
    co_await lv::sleep(1000 * ms);
    print_a_second();
  }
}

std::string status_line(const job& j) {
  const char* word = j.printing ? "PRINTING" : "WAITING...";
  return j.progress > 0 ? std::to_string(j.progress) + "% - " + word : word;
}

}  // namespace printer
