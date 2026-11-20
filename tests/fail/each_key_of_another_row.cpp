#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track_id {
  int value;
};
struct track {
  track_id id;
};
struct song {
  track_id id;
};
constinit lv::list<track, 4> tracks;
constexpr auto ui = lv::column[lv::each(tracks, &song::id)[lv::label()]];
