// A home screen with four tiles, and the lines of a job: the compiler writes both.
// docs/repetition.md walks through this file.
// editor-check: skip, it has annotations ([[=lv::caption{...}]]), which Clang does not read yet

#include <lv/list.hpp>
#include <lv/lv.hpp>
#include <lv/repeat.hpp>

using namespace lv::units;

namespace {

//! [tiles]
enum class page { none, print, scan, copy, usb };
constinit lv::state<page> shown{page::none};

// The label of a tile is a member of `labels` with the name of the enumerator.
struct labels {
  static constexpr char print[] = "Print";
  static constexpr char scan[] = "Scan";
  static constexpr char copy[] = "Copy";
  static constexpr char usb[] = "USB";
};

constexpr auto tile = []<page Page> {
  return lv::button({.on_clicked = [] { shown = Page; }, .size = {80 * px, 60 * px}})[
      lv::label({.text = lv::like<Page, labels>})];
};

constexpr auto home = lv::row({.size = {100 * pct, lv::size_content}})[
    lv::for_each<page, lv::but<page::none>>(tile)];
//! [tiles]

//! [lines]
struct job_id {
  int value;
};

struct job {
  job_id id;
  const char* title;
  [[=lv::caption{"Owner"}]] const char* owner;
  [[=lv::caption{"Pages"}]] lv::counter pages;
  [[=lv::caption{"Size"}]] const char* size;
};

constexpr auto line = []<lv::field F> {
  return lv::row[lv::label({.text = F.caption}), lv::label({.text = F.text()})];
};

constinit lv::list<job, 4> queue;
constexpr auto details = lv::column({.size = {100 * pct, lv::size_content}})[
    lv::each(queue, &job::id)[lv::column[lv::for_each_field<job>(line)]]];
//! [lines]

//! [tubes]
struct tube {
  int percent;
};
constexpr tube levels[] = {{90}, {50}, {80}, {10}};

constexpr auto gauges = lv::row({.size = {100 * pct, lv::size_content}})[
    lv::for_each<levels>([]<tube T> { return lv::bar({.range = {0, 100}, .size = {20 * px, 80 * px}, .value = {T.percent, false}}); })];
//! [tubes]

constexpr auto screen = lv::column({.size = {100 * pct, 100 * pct}})[home, details, gauges];

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<screen>(display.screen());
  if (!root) return 1;
  if (!queue.push_back({{1}, "Report.pdf", "info@example.com", lv::counter{6}, "143 kB"})) return 2;
  app.step();
  return 0;
}
