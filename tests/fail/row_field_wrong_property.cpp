#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track_id {
  int value;
};
struct track {
  track_id id;
  const char* title;
};
constinit lv::list<track, 4> tracks;
constexpr auto ui = lv::column[lv::each(tracks, &track::id)[lv::bar({.value = lv::bind(&track::title)})]];
