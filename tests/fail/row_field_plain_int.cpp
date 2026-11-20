#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track {
  int plays;
};
constexpr auto plays = lv::bind(&track::plays, "{} plays");
