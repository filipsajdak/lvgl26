#include <lv/lv.hpp>
struct stamp {
  int storage = 0;  // not a template
  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, lv::detail::build_state&) {
    return parent;
  }
};
template <>
inline constexpr bool lv::detail::is_structural<stamp> = true;
constexpr auto ui = lv::column[stamp{}];
void show(lv::obj screen) { auto root = lv::mount<ui>(screen); }
