// lv/lv.hpp alone: the error names the header of the name (lv/detail/missing.hpp).
#include <lv/lv.hpp>

struct track {
  int id;
};
constexpr int tracks = 0;
constexpr auto rows = lv::each(tracks, &track::id);
