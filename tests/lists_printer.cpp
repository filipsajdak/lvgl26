// The print queue of the printer port (examples/ports/slint/printer/queue_view.cpp), built two
// ways: as the port builds it, mounting a card for each job again after every change of the
// queue, and with lv::each, which touches the card of the changed job only. The same cards,
// the same styles and a task on each card. Prints the heap a card costs and how many cards a
// change deletes, and checks that lv::each costs less of both.

#include "check.hpp"

#include <fonts.hpp>
#include <pages.hpp>
#include <parts.hpp>
#include <queue.hpp>

#include <lv/lv.hpp>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

using namespace printer;

constexpr int column_width = page_width - 3 * margin - 2 * 162 - gap;
constexpr int card_width = column_width - 2 * margin;

constexpr auto small_font = type(tok.text.small);
constexpr auto status_font = type(tok.text.status);
constexpr auto detail_font = type(tok.text.detail);

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

// A tap on a card shows or hides what is under its title: the task of the port.
lv::task<> open_on_tap(lv::obj card_view, lv::obj details) {
  bool open = false;
  for (;;) {
    co_await card_view.next().clicked;
    open = !open;
    details.set({.hidden = !open});
  }
}

// ---- as the port does it -----------------------------------------------------

constexpr auto detail_row(std::string_view id, const char* name) {
  return lv::group({.flex_flow = "row", .size = {100 * pct, lv::size_content}, .style = {.pad_gap = tok.space.gap}})[
      lv::label({.style = {.text_color = tok.label.detail, .text_font = detail_font}, .text = name}),
      lv::label({.id = id, .style = {.text_color = tok.label.value, .text_font = detail_font}, .text = ""})];
}

template <bool Printing>
constexpr auto card = lv::group({.flex_flow = "column", .size = {card_width * px, lv::size_content},
                                  .style = {.bg_color = tok.queue.card, .bg_opa = tok.solid,
                                            .border_color = tok.queue.card_border, .border_width = tok.control.thin,
                                            .pad_all = tok.space.card, .pad_gap = tok.space.gap,
                                            .radius = tok.queue.radius}})[
    lv::group({.flex_align = {"start", "center", "center"}, .flex_flow = "row", .size = {100 * pct, lv::size_content},
               .style = {.pad_gap = tok.space.gap}})[
        lv::group({.size = {8 * px, 8 * px},
                   .style = {.bg_color = Printing ? tok.label.secondary : tok.label.tertiary, .bg_opa = tok.solid,
                             .radius = tok.queue.dot}}),
        lv::label({.id = "status",
                   .style = {.text_color = Printing ? tok.label.secondary : tok.label.tertiary,
                             .text_font = status_font},
                   .text = ""})],
    lv::label({.id = "title", .size = {100 * pct, lv::size_content},
               .style = {.text_color = tok.label.primary, .text_font = small_font}, .text = ""}),
    lv::group({.flex_flow = "column", .hidden = true, .id = "details", .size = {100 * pct, lv::size_content}})[
        detail_row("owner", "Job Owner"), detail_row("pages", "Pages"), detail_row("size", "Size"),
        detail_row("submitted", "Submitted")]];

constexpr auto column = lv::group({.flex_flow = "column", .size = {column_width * px, 400 * px}});

std::vector<lv::owned<lv::obj>> cards;

template <const auto& Tree>
void add_card(lv::obj parent, const job& j) {
  auto built = lv::mount<Tree>(parent);
  if (!built) return;
  built->template get<"status">().template set<"text">(status_line(j).c_str());
  built->template get<"title">().template set<"text">(j.title.c_str());
  built->template get<"owner">().template set<"text">(j.owner.c_str());
  built->template get<"pages">().template set<"text">(std::to_string(j.pages).c_str());
  built->template get<"size">().template set<"text">(j.size.c_str());
  built->template get<"submitted">().template set<"text">(j.submitted.c_str());
  const lv::obj root = built->root();
  lv::spawn(open_on_tap(root, built->template get<"details">()), root);
  cards.emplace_back(built->detach());
}

void rebuild(lv::obj list, const std::vector<job>& jobs) {
  cards.clear();
  for (const job& j : jobs) {
    if (j.printing)
      add_card<card<true>>(list, j);
    else
      add_card<card<false>>(list, j);
  }
}

// ---- with lv::each ---------------------------------------------------------------

struct job_id {
  int value;
};

// A job as the queue holds it: what a card shows, as text, and whether it prints.
struct queued {
  job_id id;
  std::string title;
  std::string owner;
  std::string size;
  std::string submitted;
  lv::counter pages;
  std::string status;
  bool printing;
};

constinit lv::list<queued, 8> queue;

constexpr auto detail_line(const char* name, auto value) {
  return lv::group({.flex_flow = "row", .size = {100 * pct, lv::size_content}, .style = {.pad_gap = tok.space.gap}})[
      lv::label({.style = {.text_color = tok.label.detail, .text_font = detail_font}, .text = name}),
      lv::label({.style = {.text_color = tok.label.value, .text_font = detail_font}, .text = value})];
}

//! [card]
// One template for both looks: a printing job's card is checked, and its state reaches the
// dot and the status line, which style themselves for it.
constexpr auto job_card =
    lv::group({.checked = lv::bind(&queued::printing), .flex_flow = "column",
               .size = {card_width * px, lv::size_content}, .state_trickle = true,
               .style = {.bg_color = tok.queue.card, .bg_opa = tok.solid,
                         .border_color = tok.queue.card_border, .border_width = tok.control.thin,
                         .pad_all = tok.space.card, .pad_gap = tok.space.gap, .radius = tok.queue.radius}})[
        lv::group({.flex_align = {"start", "center", "center"}, .flex_flow = "row",
                   .size = {100 * pct, lv::size_content}, .state_trickle = true, .style = {.pad_gap = tok.space.gap}})[
            lv::group({.size = {8 * px, 8 * px},
                       .style = {.bg_color = tok.label.tertiary, .bg_opa = tok.solid, .radius = tok.queue.dot},
                       .style_checked = {.bg_color = tok.label.secondary}}),
            lv::label({.style = {.text_color = tok.label.tertiary, .text_font = status_font},
                       .style_checked = {.text_color = tok.label.secondary},
                       .text = lv::bind(&queued::status)})],
        lv::label({.size = {100 * pct, lv::size_content},
                   .style = {.text_color = tok.label.primary, .text_font = small_font},
                   .text = lv::bind(&queued::title)}),
        lv::group({.flex_flow = "column", .hidden = true, .id = "details", .size = {100 * pct, lv::size_content}})[
            detail_line("Job Owner", lv::bind(&queued::owner)), detail_line("Pages", lv::bind(&queued::pages, "{}")),
            detail_line("Size", lv::bind(&queued::size)), detail_line("Submitted", lv::bind(&queued::submitted))]];

void start_card(lv::obj row, const queued&) {
  lv::spawn(open_on_tap(row, lv::row_view<job_card>{row}.get<"details">()), row);
}

constexpr auto queue_column = lv::group({.flex_flow = "column", .size = {column_width * px, 400 * px}})[
    lv::each(queue, &queued::id, {.on_row = &start_card})[job_card]];
//! [card]

queued from(const job& j, int id) {
  return {{id}, j.title, j.owner, j.size, j.submitted, lv::counter{j.pages}, status_line(j), j.printing};
}

std::vector<job> sample() {
  return {{.title = "210106-FinalPresentation.pdf", .pages = 6, .progress = 63, .printing = true},
          {.title = "Adressliste.docx", .pages = 6},
          {.title = "Salaries.pdf", .pages = 6},
          {.title = "EW 2026 Slides", .size = "2.5MB", .pages = 300},
          {.title = "Report.pdf", .pages = 12},
          {.title = "Menu.pdf", .pages = 2}};
}

// How many of these widgets are gone.
std::size_t gone(const std::vector<lv::weak<lv::obj>>& w) {
  std::size_t n = 0;
  for (const auto& x : w) n += x.expired() ? std::size_t{1} : std::size_t{0};
  return n;
}

std::vector<lv::weak<lv::obj>> watch_children(lv::obj parent) {
  std::vector<lv::weak<lv::obj>> out;
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(parent.raw()); ++i)
    out.emplace_back(lv::obj{lv_obj_get_child(parent.raw(), static_cast<std::int32_t>(i))});
  return out;
}

struct outcome {
  std::size_t heap_per_card;
  std::size_t deleted_by_progress;  // the first job gains a percent
  std::size_t deleted_by_new_job;   // a job is added at the end
  std::size_t deleted_by_cancel;    // the second job is cancelled
};

outcome remounted(lv::obj screen, std::vector<job> jobs) {
  outcome o{};
  auto list = lv::mount<column>(screen);
  if (!list) return o;
  const lv::obj l = list->root();
  const std::size_t empty = free_bytes();
  rebuild(l, jobs);
  o.heap_per_card = (empty - free_bytes()) / jobs.size();

  auto w = watch_children(l);
  jobs.front().progress += 1;
  rebuild(l, jobs);
  o.deleted_by_progress = gone(w);

  w = watch_children(l);
  jobs.push_back({.title = "New.pdf"});
  rebuild(l, jobs);
  o.deleted_by_new_job = gone(w);

  w = watch_children(l);
  jobs.erase(jobs.begin() + 1);
  rebuild(l, jobs);
  o.deleted_by_cancel = gone(w);
  cards.clear();
  return o;
}

outcome with_each(lv::obj screen, const std::vector<job>& jobs) {
  outcome o{};
  queue.clear();
  auto list = lv::mount<queue_column>(screen);
  if (!list) return o;
  const lv::obj l = list->root();
  const std::size_t empty = free_bytes();
  for (std::size_t i = 0; i < jobs.size(); ++i)
    check(queue.push_back(from(jobs[i], static_cast<int>(i + 1))).has_value());
  o.heap_per_card = (empty - free_bytes()) / jobs.size();

  auto w = watch_children(l);
  queue.update(0, [](queued& q) { q.status = "64% - PRINTING"; });
  o.deleted_by_progress = gone(w);
  lv_obj_t* status = lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(l.raw(), 0), 0), 1);
  check(std::strcmp(lv_label_get_text(status), "64% - PRINTING") == 0);

  w = watch_children(l);
  check(queue.push_back(from(job{.title = "New.pdf"}, 99)).has_value());
  o.deleted_by_new_job = gone(w);

  w = watch_children(l);
  queue.erase(1);
  o.deleted_by_cancel = gone(w);
  queue.clear();
  return o;
}

// The printing card is checked, and so are its dot and its status line; a waiting one is not.
void one_template_has_both_looks(lv::obj screen) {
  queue.clear();
  auto list = lv::mount<queue_column>(screen);
  if (!list) return check(false);
  const auto jobs = sample();
  check(queue.push_back(from(jobs[0], 1)).has_value() && queue.push_back(from(jobs[1], 2)).has_value());
  lv_obj_t* printing = lv_obj_get_child(list->root().raw(), 0);
  lv_obj_t* waiting = lv_obj_get_child(list->root().raw(), 1);
  lv_obj_t* dot = lv_obj_get_child(lv_obj_get_child(printing, 0), 0);
  check(lv_obj_has_state(printing, LV_STATE_CHECKED) && lv_obj_has_state(dot, LV_STATE_CHECKED));
  check(!lv_obj_has_state(waiting, LV_STATE_CHECKED));
  lv_obj_t* waiting_dot = lv_obj_get_child(lv_obj_get_child(waiting, 0), 0);
  check(!lv_color_eq(lv_obj_get_style_bg_color(dot, LV_PART_MAIN), lv_obj_get_style_bg_color(waiting_dot, LV_PART_MAIN)));
  // The job starts printing: its card turns.
  queue.update(1, [](queued& q) { q.printing = true; });
  check(lv_obj_has_state(lv_obj_get_child(list->root().raw(), 1), LV_STATE_CHECKED));
  // A tap opens the details of that card alone.
  lv_obj_send_event(waiting, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* details = lv_obj_get_child(waiting, 2);
  check(!lv_obj_is_hidden(details) && lv_obj_is_hidden(lv_obj_get_child(printing, 2)));
  queue.clear();
}

}  // namespace

constinit lv::frame<window_width, window_height> frame;

int main() {
  lv::application app;
  lv::display display{app, frame};
  const lv::obj screen = display.screen();

  one_template_has_both_looks(screen);
  // Once each way first: the first mount makes what later mounts share.
  remounted(screen, sample());
  with_each(screen, sample());
  const outcome before = remounted(screen, sample());
  const outcome after = with_each(screen, sample());

  std::printf("heap per card: remounted %zu, lv::each %zu bytes\n", before.heap_per_card, after.heap_per_card);
  std::printf("cards deleted by a percent: %zu and %zu\n", before.deleted_by_progress, after.deleted_by_progress);
  std::printf("cards deleted by a new job: %zu and %zu\n", before.deleted_by_new_job, after.deleted_by_new_job);
  std::printf("cards deleted by a cancel: %zu and %zu\n", before.deleted_by_cancel, after.deleted_by_cancel);

  check(after.heap_per_card < before.heap_per_card);
  check(before.deleted_by_progress == 6 && after.deleted_by_progress == 0);
  check(before.deleted_by_new_job == 6 && after.deleted_by_new_job == 0);
  check(before.deleted_by_cancel == 7 && after.deleted_by_cancel == 1);
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
