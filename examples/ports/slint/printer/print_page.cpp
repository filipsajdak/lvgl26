// The Print page: the whole queue, a wide card for each job with its details, Pause and Delete.

#include <lv/list.hpp>

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue.hpp"

namespace printer {
namespace {

constexpr int card_width = page_width - 2 * margin;
constexpr int card_height = 136;
constexpr int left_width = 330;
constexpr int bar_width = 164;

constexpr auto title_font = type(tok.text.title);
constexpr auto body_font = type(tok.text.body);
constexpr auto subtitle_font = type(tok.text.subtitle);
constexpr auto status_font = type(tok.text.status);
constexpr auto detail_font = type(tok.text.detail);

constexpr auto detail_row(const char* name, auto value, int y) {
  return lv::group({.size = {240 * px, 20 * px}, .x = 0 * px, .y = y * px})[
      lv::label({.text = name, .style = {.text_color = tok.label.detail, .text_font = detail_font}}),
      lv::label({.align = "right_mid", .text = value,
                 .style = {.text_color = tok.label.value, .text_font = detail_font}})];
}

constexpr auto rule(int y) {
  return lv::group({.size = {240 * px, 1 * px}, .y = y * px, .style = {.bg_color = tok.queue.rule, .bg_opa = tok.solid}});
}

// Pause does nothing in the original either.
void nothing() {}

// A tap on the Delete button of a card removes that card's job. The task ends with the card.
lv::task<> delete_on_tap(lv::obj button, job_id id) {
  co_await button.next().clicked;
  cancel_job(id);
}

//! [card]
constexpr auto card = lv::group({.size = {card_width * px, card_height * px},
                                 .style = {.bg_color = tok.window.background, .bg_opa = tok.solid,
                                           .border_color = tok.color.neutral, .border_width = tok.control.line,
                                           .radius = tok.queue.wide_radius}})[
    lv::label({.text = lv::bind(&job::status),
               .x = 14 * px, .y = 14 * px, .style = {.text_color = tok.label.secondary, .text_font = status_font}}),
    lv::group({.size = {bar_width * px, tok.queue.track},
               .x = (left_width - bar_width) * px,
               .y = 18 * px, .style = {.bg_color = tok.color.neutral, .bg_opa = tok.solid, .radius = tok.queue.dot}})[
        // The ink grows with the share of the job that is done: LVGL rounds a grown item to the nearest
        // pixel, where the original cuts it down, so the span is one pixel short to stay within a pixel.
        lv::group({.flex_flow = "row", .size = {(bar_width - 1) * px, 100 * pct}})[
            lv::group({.flex_grow = lv::bind(&job::progress), .size = {0 * px, 100 * pct},
                       .style = {.bg_color = tok.color.foreground, .bg_opa = tok.solid,
                                 .min_width = tok.queue.track, .radius = tok.queue.dot}}),
            lv::group({.flex_grow = lv::bind(&job::left), .size = {0 * px, 100 * pct}})]],
    lv::label({.size = {left_width * px, lv::size_content},
               .text = lv::bind(&job::title),
               .x = 14 * px, .y = 38 * px, .style = {.text_color = tok.label.primary, .text_font = subtitle_font}}),
    lv::group({.x = 14 * px, .y = 82 * px})[
        icon_text_button<icons::pause>("pause", "Pause", &nothing, tok.button.primary, body_font)],
    lv::group({.x = (14 + 110 + 14) * px, .y = 82 * px})[
        icon_text_button<icons::bin>("delete", "Delete", &nothing, tok.button.destructive, body_font)],
    lv::group({.size = {240 * px, 100 * px}, .x = (card_width - 240 - 14) * px, .y = 14 * px})[
        detail_row("Job Owner", lv::bind(&job::owner), 0), rule(24),
        detail_row("Pages", lv::bind(&job::pages, "{}"), 28), rule(52),
        detail_row("Size", lv::bind(&job::size), 56), rule(80),
        detail_row("Submitted", lv::bind(&job::submitted), 84)]];

void start_card(lv::obj row, const job& j) {
  lv::spawn(delete_on_tap(lv::obj{lv::row_view<card>{row}.get<"delete">().raw()}, j.id), row);
}

constexpr auto ui = lv::group({.size = {100 * pct, 100 * pct},
                               .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
    back_header("Print", "Printing-Queue", title_font, body_font),
    lv::group({.flex_flow = "column", .size = {100 * pct, (window_height - header_height) * px},
               .y = header_height * px, .style = {.pad_all = tok.space.edge, .pad_gap = tok.space.card}})[
        lv::each(queue, &job::id, {.on_row = &start_card})[card]]];
//! [card]

}  // namespace

constinit const lv::page print_page = lv::page_of<ui>;

}  // namespace printer
