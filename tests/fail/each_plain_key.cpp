#include <lv/lv.hpp>
struct track {
  int id;
  const char* title;
};
constinit lv::list<track, 4> tracks;
constexpr auto ui = lv::column[lv::each(tracks, &track::id)[lv::label()]];
