#pragma once

// The print queue as a column on the home page: a card for each job, which a tap opens.
// It is a tree of its own, which the home page nests, so the cards live and die with the page.

#include <lv/list.hpp>

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue.hpp"

namespace printer::queue_part {

constexpr int column_width = page_width - 3 * margin - 2 * tile_width - gap;
constexpr int card_width   = column_width - 2 * margin;
constexpr int list_top     = 70;

constexpr auto heading_font = type(tok.text.heading);
constexpr auto small_font   = type(tok.text.small);
constexpr auto status_font  = type(tok.text.status);
constexpr auto detail_font  = type(tok.text.detail);

// One line of the details: the name of a value, and the value.
constexpr auto detail_line(const char* name, auto value) {
  return lv::group(
    {
      .flex_flow = "row",
      .size      = {100 * pct, lv::size_content},
      .style     = {.pad_gap = tok.space.gap},
    },
    lv::label({
      .text  = name,
      .style = {.text_color = tok.label.detail, .text_font = detail_font},
    }),
    lv::label({
      .text  = value,
      .style = {.text_color = tok.label.value, .text_font = detail_font},
    })
  );
}

//! [card]
// One card for every job. The dot and the words are blue while the job prints and gray while it
// waits. A tap checks the card, and the checked card shows the details under its title.
constexpr auto printing_dot = lv::group({
  .checked = lv::bind(&job::printing),
  .size    = {8 * px, 8 * px},
  .style = {
    .bg_color = tok.label.tertiary,
    .bg_opa   = tok.solid,
    .radius   = tok.queue.dot,
  },
  .style_checked = {.bg_color = tok.label.secondary},
});

constexpr auto status_line = lv::label({
  .checked       = lv::bind(&job::printing),
  .text          = lv::bind(&job::status),
  .style         = {.text_color = tok.label.tertiary, .text_font = status_font},
  .style_checked = {.text_color = tok.label.secondary},
});

constexpr auto status_row = lv::group(
  {
    .flex_align = {"start", "center", "center"},
    .flex_flow  = "row",
    .size       = {100 * pct, lv::size_content},
    .style      = {.pad_gap = tok.space.gap},
  },
  printing_dot, status_line
);

constexpr auto title_line = lv::label({
  .size  = {100 * pct, lv::size_content},
  .text  = lv::bind(&job::title),
  .style = {.text_color = tok.label.primary, .text_font = small_font},
});

// The details are folded away until the card is checked.
constexpr auto details = lv::group(
  {
    .flex_flow     = "column",
    .size          = {100 * pct, lv::size_content},
    .style         = {.max_height = tok.space.none},
    .style_checked = {.max_height = lv::raw(window_height * px), .pad_top = tok.space.gap},
  },
  detail_line("Job Owner", lv::bind(&job::owner)),
  detail_line("Pages", lv::bind(&job::pages, "{}")), detail_line("Size", lv::bind(&job::size)),
  detail_line("Submitted", lv::bind(&job::submitted))
);

constexpr auto title_and_details = lv::group(
  {
    .flex_flow     = "column",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
  },
  title_line, details
);

constexpr auto card = lv::group(
  {
    .checkable     = true,
    .flex_flow     = "column",
    .size          = {card_width * px, lv::size_content},
    .state_trickle = true,
    .style = {
      .bg_color     = tok.queue.card,
      .bg_opa       = tok.solid,
      .border_color = tok.queue.card_border,
      .border_width = tok.control.thin,
      .pad_all      = tok.space.card,
      .pad_gap      = tok.space.gap,
      .radius       = tok.queue.radius,
    },
  },
  status_row, title_and_details
);
//! [card]

constexpr auto queue_column = lv::group(
  {
    .size  = {column_width * px, 100 * pct},
    .x     = (page_width - column_width) * px,
    .style = {.bg_color = tok.queue.background, .bg_opa = tok.solid},
  },
  lv::group({
    .size  = {1 * px, 100 * pct},
    .style = {.bg_color = tok.queue.separator, .bg_opa = tok.solid},
  }),
  lv::label({
    .text  = "Printing Queue",
    .x     = margin * px,
    .y     = margin * px,
    .style = {.text_color = tok.label.primary, .text_font = heading_font},
  }),
  lv::group(
    {
      .flex_flow = "column",
      .size      = {100 * pct, (window_height - list_top) * px},
      .y         = list_top * px,
      .style     = {.pad_all = tok.space.edge, .pad_gap = tok.space.card},
    },
    lv::each(queue, &job::id, card)
  )
);

}  // namespace printer::queue_part
