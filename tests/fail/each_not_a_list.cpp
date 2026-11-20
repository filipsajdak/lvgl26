#include <lv/list.hpp>
#include <lv/lv.hpp>
#include <vector>
struct track_id {
  int value;
};
struct track {
  track_id id;
};
const std::vector<track> tracks;
constexpr auto ui = lv::column[lv::each(tracks, &track::id)[lv::label()]];
