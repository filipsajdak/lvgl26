#pragma once

// The print queue: the jobs, and what the buttons do to them. The two views of the queue (the
// column on the home page and the list of the Print page) are `lv::each` over `queue`. A change
// of a job touches the widgets of that job only.

#include <lv/list.hpp>
#include <lv/lv.hpp>

#include <cstddef>

namespace printer {

// Names a job. No job has the id 0.
struct job_id {
  int value;
};

inline constexpr std::size_t queue_capacity = 32;

struct job {
  job_id id;
  const char* title;
  const char* owner = "joe@example.com";
  const char* size = "100kB";
  const char* submitted = "11:41 25/01/26";
  lv::counter pages{1};
  lv::counter progress{};  // percent; the first job gains one a second
  lv::counter left{100};  // 100 less `progress`; the bar on the Print page grows with both
  bool printing = false;
  char status[16] = "WAITING...";  // "63% - PRINTING", "WAITING..."; follows `progress`
};

// The jobs, in the order they print.
inline constinit lv::list<job, queue_capacity> queue;

// The four jobs that the panel starts with.
void load_demo_queue();

// Adds a job to the end of the queue, as the Start buttons do. Gives its id, or 0 when the queue is full.
job_id start_job(const char* title);

// Removes a job, as the Delete button does.
void cancel_job(job_id id);

// The print head: once a second the first job gains a percent, and leaves at a hundred.
lv::task<> run_queue();

}  // namespace printer
