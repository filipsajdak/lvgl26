// The Print page: the whole queue, a wide card for each job with its details, Pause and Delete.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue.hpp"

#include <algorithm>

namespace printer {
namespace {

constexpr int card_width = page_width - 2 * margin;
constexpr int card_height = 136;
constexpr int bar_width = 164;
constexpr int left_width = 330;

constexpr auto title_font = type(tok.text.title);
constexpr auto body_font = type(tok.text.body);
constexpr auto subtitle_font = type(tok.text.subtitle);
constexpr auto status_font = type(tok.text.status);
constexpr auto detail_font = type(tok.text.detail);

constexpr auto detail_row(std::string_view id, const char* name, int y) {
  return lv::group({.size = {240 * px, 20 * px}, .x = 0 * px, .y = y * px})[
      lv::label({.style = {.text_color = tok.label.detail, .text_font = detail_font}, .text = name}),
      lv::label({.align = "right_mid", .id = id,
                 .style = {.text_color = tok.label.value, .text_font = detail_font}, .text = ""})];
}

constexpr auto rule(int y) {
  return lv::group({.size = {240 * px, 1 * px}, .style = {.bg_color = tok.queue.rule, .bg_opa = tok.solid}, .y = y * px});
}

// Pause does nothing in the original either.
void nothing() {}

lv::task<> delete_on_tap(lv::obj button, std::size_t at) {
  co_await button.next().clicked;
  cancel_job(at);
}

constexpr auto card = lv::group({.size = {card_width * px, card_height * px},
                                 .style = {.bg_color = tok.window.background, .bg_opa = tok.solid,
                                           .border_color = tok.color.neutral, .border_width = tok.control.line,
                                           .radius = tok.queue.wide_radius}})[
    lv::label({.id = "status", .style = {.text_color = tok.label.secondary, .text_font = status_font}, .text = "",
               .x = 14 * px, .y = 14 * px}),
    lv::group({.size = {bar_width * px, tok.queue.track}, .style = {.bg_color = tok.color.neutral, .bg_opa = tok.solid,
                                                                    .radius = tok.queue.dot},
               .x = (left_width - bar_width) * px, .y = 18 * px})[
        lv::group({.id = "fill", .size = {tok.queue.track, tok.queue.track},
                   .style = {.bg_color = tok.color.foreground, .bg_opa = tok.solid, .radius = tok.queue.dot}})],
    lv::label({.id = "title", .size = {left_width * px, lv::size_content},
               .style = {.text_color = tok.label.primary, .text_font = subtitle_font}, .text = "",
               .x = 14 * px, .y = 38 * px}),
    lv::group({.x = 14 * px, .y = 82 * px})[
        icon_text_button<icons::pause>("pause", "Pause", &nothing, tok.button.primary, body_font)],
    lv::group({.x = (14 + 110 + 14) * px, .y = 82 * px})[
        icon_text_button<icons::bin>("delete", "Delete", &nothing, tok.button.destructive, body_font)],
    lv::group({.size = {240 * px, 100 * px}, .x = (card_width - 240 - 14) * px, .y = 14 * px})[
        detail_row("owner", "Job Owner", 0), rule(24), detail_row("pages", "Pages", 28), rule(52),
        detail_row("size", "Size", 56), rule(80), detail_row("submitted", "Submitted", 84)]];

constexpr auto ui = lv::group({.size = {100 * pct, 100 * pct},
                               .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
    back_header("Print", "Printing-Queue", title_font, body_font),
    lv::group({.flex_flow = "column", .id = "list", .size = {100 * pct, (window_height - header_height) * px},
                .style = {.pad_all = tok.space.edge, .pad_gap = tok.space.card}, .y = header_height * px})];

std::vector<lv::owned<lv::obj>> cards;
lv::weak<lv::obj> list;

void add_card(lv::obj parent, const job& j, std::size_t at) {
  auto built = lv::mount<card>(parent);
  if (!built) return;
  built->get<"status">().set<"text">(status_line(j).c_str());
  built->get<"title">().set<"text">(j.title.c_str());
  built->get<"owner">().set<"text">(j.owner.c_str());
  built->get<"pages">().set<"text">(std::to_string(j.pages).c_str());
  built->get<"size">().set<"text">(j.size.c_str());
  built->get<"submitted">().set<"text">(j.submitted.c_str());
  built->get<"fill">().set({.width = std::max(bar_width * j.progress / 100, 6) * px});
  lv::spawn(delete_on_tap(lv::obj{built->get<"delete">().raw()}, at), built->root());
  cards.emplace_back(built->detach());
}

void rebuild(lv::counter) {
  cards.clear();
  if (!list) return;
  std::size_t at = 0;
  for (const job& j : jobs()) add_card(list.get(), j, at++);
}

constinit lv::effect follow{&rebuild, queue_revision};

}  // namespace

lv::owned<lv::obj> mount_print(lv::obj parent) {
  auto built = lv::mount<ui>(parent);
  if (!built) return {};
  list = lv::weak<lv::obj>{built->get<"list">()};
  rebuild(lv::counter{});
  follow.start();
  return lv::owned<lv::obj>{built->detach()};
}

}  // namespace printer
