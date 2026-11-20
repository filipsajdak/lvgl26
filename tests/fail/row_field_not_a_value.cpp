#include <lv/list.hpp>
#include <lv/lv.hpp>
struct place {
  int x;
  int y;
};
struct track {
  place where;
};
constexpr auto where = lv::bind(&track::where);
