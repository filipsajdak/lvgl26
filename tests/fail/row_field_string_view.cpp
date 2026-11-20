#include <lv/list.hpp>
#include <lv/lv.hpp>
#include <string_view>
struct track {
  std::string_view title;
};
constexpr auto title = lv::bind(&track::title);
