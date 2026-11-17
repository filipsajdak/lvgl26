// The print queue as a column on the home page: a card for each job, which a tap opens.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue.hpp"

namespace printer {
namespace {

constexpr int column_width = page_width - 3 * margin - 2 * 162 - gap;
constexpr int card_width = column_width - 2 * margin;
constexpr int card_pad = 16;
constexpr int list_top = 70;

constexpr auto heading_font = type(tok.text.heading);
constexpr auto small_font = type(tok.text.small);
constexpr auto status_font = type(tok.text.status);
constexpr auto detail_font = type(tok.text.detail);

// One line of the details: the name of a value, and the value. The value is set when the card is made.
constexpr auto detail_row(std::string_view id, const char* name) {
  return lv::group({.flex_flow = "row", .size = {100 * pct, lv::size_content}, .style = {.pad_gap = tok.space.gap}})[
      lv::label({.style = {.text_color = tok.label.detail, .text_font = detail_font}, .text = name}),
      lv::label({.id = id, .style = {.text_color = tok.label.value, .text_font = detail_font}, .text = ""})];
}

// A card. The dot and the words are blue while the job prints, gray while it waits.
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

constexpr auto ui = lv::group({.size = {column_width * px, 100 * pct},
                               .style = {.bg_color = tok.queue.background, .bg_opa = tok.solid},
                               .x = (page_width - column_width) * px})[
    lv::group({.size = {1 * px, 100 * pct}, .style = {.bg_color = tok.queue.separator, .bg_opa = tok.solid}}),
    lv::label({.style = {.text_color = tok.label.primary, .text_font = heading_font},
               .text = "Printing Queue", .x = margin * px, .y = margin * px}),
    lv::group({.flex_flow = "column", .id = "list", .size = {100 * pct, (window_height - list_top) * px},
                .style = {.pad_all = tok.space.edge, .pad_gap = tok.space.card}, .y = list_top * px})];

std::vector<lv::owned<lv::obj>> cards;
lv::weak<lv::obj> list;

// A tap on a card shows or hides what is under its title.
lv::task<> open_on_tap(lv::obj card_view, lv::obj details) {
  bool open = false;
  for (;;) {
    co_await card_view.next().clicked;
    open = !open;
    details.set({.hidden = !open});
  }
}

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

//! [rebuild]
void rebuild(lv::counter) {
  cards.clear();
  if (!list) return;
  for (const job& j : jobs()) {
    if (j.printing)
      add_card<card<true>>(list.get(), j);
    else
      add_card<card<false>>(list.get(), j);
  }
}

constinit lv::effect follow{&rebuild, queue_revision};
//! [rebuild]

}  // namespace

lv::owned<lv::obj> mount_queue_view(lv::obj parent) {
  auto built = lv::mount<ui>(parent);
  if (!built) return {};
  list = lv::weak<lv::obj>{built->get<"list">()};
  rebuild(lv::counter{});
  follow.start();
  return lv::owned<lv::obj>{built->detach()};
}

}  // namespace printer
