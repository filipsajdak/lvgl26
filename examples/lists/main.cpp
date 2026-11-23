// A playlist: rows that a program adds, moves and changes, and the selected row.
// docs/lists.md walks through this file.
//
//   lists    writes lists.png

//! [includes]
#include <lv/list.hpp>
#include <lv/lv.hpp>
//! [includes]
#include <lv/png.hpp>

using namespace lv::units;

namespace {

constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr auto tok = lv::tokens<tokens_json>;

//! [model]
struct track_id {
  int value;
};

struct track {
  track_id id;  // names the row
  const char* title;
  lv::counter plays;
};

constinit lv::list<track, 8> tracks;       // room for eight rows
constinit lv::state current{track_id{0}};  // the selected row; no track has the id 0
//! [model]

//! [row]
// One row: a button that is checked while it is the selected one. state_trickle hands that
// state to the labels, which style themselves for it.
constexpr auto track_row = lv::button(
  {
    .flex_flow     = "row",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
    .style = {
      .bg_color = tok.color.row,
      .pad_all  = tok.space.edge,
      .radius   = tok.radius.row,
    },
    .style_checked = {.bg_color = tok.color.selected},
  },
  lv::label({
    .flex_grow     = 1,
    .text          = lv::bind(&track::title),
    .style         = {.text_color = tok.color.text},
    .style_checked = {.text_color = tok.color.on_selected},
  }),
  lv::label({
    .text          = lv::bind(&track::plays, "{} plays"),
    .style         = {.text_color = tok.color.quiet},
    .style_checked = {.text_color = tok.color.on_selected},
  })
);
//! [row]

//! [tree]
constinit lv::computed has_tracks{
  [](lv::counter n) {
    return n.value > 0;
  },
  tracks.count(),
};

// Shown while the list is empty.
constexpr auto empty_note = lv::label({
  .hidden = lv::bind(has_tracks),
  .text   = "No tracks",
  .style  = {.text_color = tok.color.quiet},
});

// One row per track; the selected one is checked.
constexpr auto track_list = lv::group(
  {
    .flex_flow = "column",
    .size      = {100 * pct, lv::size_content},
    .style     = {.pad_gap = tok.space.gap},
  },
  lv::each(tracks, &track::id, {.selected = lv::bind(current)}, track_row)
);

// Groups: boxes without the theme's look, laid out as columns.
constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .size      = {320 * px, 240 * px},
    .style = {
      .bg_color = tok.color.screen,
      .bg_opa   = tok.opacity.opaque,
      .pad_all  = tok.space.edge,
      .pad_gap  = tok.space.gap,
    },
  },
  empty_note, track_list
);
//! [tree]

constinit lv::frame<320, 240> frame;

//! [rows]
// The playlist to start with, in one constant.
constexpr track playlist[] = {
  {{4}, "Overture", lv::counter{0}},
  {{1}, "Intro", lv::counter{3}},
  {{2}, "Northern Lights", lv::counter{1}},
  {{3}, "Harbour", lv::counter{12}}
};
//! [rows]

//! [changes]
void play(std::size_t row) {
  tracks.update(row, [](track& t) {
    ++t.plays;
  });  // that row's labels are set again
}
//! [changes]

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  //! [fill]
  if (!tracks.assign(playlist)) return 2;  // builds a row for each track: full, or no memory
  //! [fill]
  //! [move]
  play(2);
  tracks.move(3, 1);      // Harbour moves up, with its widgets
  current = track_id{2};  // selects Northern Lights
  //! [move]
  app.step();
  return lv::write_png(display.screen(), "lists.png") ? 0 : 3;
}
