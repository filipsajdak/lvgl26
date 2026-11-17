#include <lv/lv.hpp>
struct track_id {
  int value;
};
struct track {
  track_id id;
  bool done;
};
constinit lv::list<track, 4> tracks;
constinit lv::state current{track_id{0}};
constexpr auto ui = lv::column[
    lv::each(tracks, &track::id, {.selected = lv::bind(current)})[lv::button({.checked = lv::bind(&track::done)})]];
void f(lv::obj screen) { lv::mount<ui>(screen); }
