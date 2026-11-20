#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track {
  const char* name() const;
};
constexpr auto name = lv::bind(&track::name);
