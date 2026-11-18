#include <lv/lv.hpp>
struct stamp {
  template <const auto& Tree, std::size_t... Path>
  struct storage {
    int mark = 0;
  };
  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, lv::detail::build_state&) {
    return parent;
  }
};
template <>
inline constexpr bool lv::detail::is_structural<stamp> = true;
constexpr auto ui = lv::column[stamp{}];
lv::obj give_away(lv::obj screen) { return lv::place<ui>(screen); }
