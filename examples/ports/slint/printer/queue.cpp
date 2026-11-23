#include "queue.hpp"

#include <format>

namespace printer {
namespace {

using namespace lv::units;

constexpr int percent_done = 100;

constexpr job demo_jobs[] = {
  {
    .id    = {1},
    .title = "210106-FinalPresentation.pdf",
    .owner = "info@example.com",
    .size  = "143kb",
    .pages{6},
    .progress{63},
    .left{37},
    .printing = true,
    .status   = "63% - PRINTING",
  },
  {
    .id    = {2},
    .title = "Adressliste.docx",
    .owner = "info@example.com",
    .size  = "143kb",
    .pages{6},
  },
  {
    .id    = {3},
    .title = "Salaries.pdf",
    .owner = "info@example.com",
    .size  = "143kb",
    .pages{6},
  },
  {
    .id        = {4},
    .title     = "EW 2026 Slides",
    .owner     = "info@example.com",
    .size      = "2.5MB",
    .submitted = "11:41 10/03/26",
    .pages{300},
  },
};

int next_id = 5;

// The job gains a percent: its text and its bar follow.
void advance(job& j) {
  ++j.progress.value;
  --j.left.value;
  j.printing                                                                               = true;
  *std::format_to_n(j.status, sizeof j.status - 1, "{}% - PRINTING", j.progress.value).out = '\0';
}

// One second of printing: the first job gains a percent, and leaves at a hundred.
void print_a_second() {
  if (queue.empty()) return;
  if (queue[0].progress.value >= percent_done)
    queue.erase(0);
  else
    queue.update(0, &advance);
}

}  // namespace

void load_demo_queue() { (void)queue.assign(demo_jobs); }

job_id start_job(const char* title) {
  const job_id id{next_id};
  if (!queue.push_back({.id = id, .title = title})) return {};
  ++next_id;
  return id;
}

void cancel_job(job_id id) {
  for (std::size_t i = 0; i < queue.size(); ++i)
    if (queue[i].id.value == id.value) return queue.erase(i);
}

lv::task<> run_queue() {
  for (;;) {
    co_await lv::sleep(1000 * ms);
    print_a_second();
  }
}

}  // namespace printer
