#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track_id {
  int value;
};
struct track {
  track_id id;
};
constinit lv::list<track, 0> tracks;
