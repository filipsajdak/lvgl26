#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track_id {
  int value;
};
struct page {
  int value;
};
struct track {
  track_id id;
};
constinit lv::list<track, 4> tracks;
constinit lv::state current{page{0}};
constexpr auto ui = lv::column[lv::each(tracks, &track::id, {.selected = lv::bind(current)})[lv::label()]];
