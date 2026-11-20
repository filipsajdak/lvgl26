#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track_id {
  int value;
};
struct track {
  track_id id;
};
constinit lv::list<track, 4> tracks;
constexpr auto ui = lv::column[lv::each(tracks, &track::id)[lv::row[lv::label({.id = "a"}), lv::label({.id = "a"})]]];
void f(lv::obj screen) { lv::mount<ui>(screen); }
