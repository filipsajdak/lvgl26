#pragma once

// The print queue: the jobs, and what the buttons do to them. The two views of the queue
// (the column on the home page and the list of the Print page) show this, and both follow
// `queue_revision`.

#include <lv/lv.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace printer {

struct job {
  std::string title;
  std::string owner = "joe@example.com";
  std::string size = "100kB";
  std::string submitted = "11:41 25/01/26";
  int pages = 1;
  int progress = 0;  // percent; the first job gains one a second
  bool printing = false;
};

// Changes whenever the jobs do.
inline constinit lv::state<lv::counter> queue_revision{lv::counter{0}};

// The jobs, in the order they print.
const std::vector<job>& jobs();

// Adds a job to the end of the queue, as the Start buttons do.
void start_job(const char* title);

// Removes a job, as the Delete button does.
void cancel_job(std::size_t index);

// The print head: once a second the first job gains a percent, and leaves at a hundred.
lv::task<> run_queue();

// The line that tells a job's state: "63% - PRINTING", "WAITING...".
std::string status_line(const job& j);

}  // namespace printer
