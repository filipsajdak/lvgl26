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
struct row_id {
  int value;
};
struct row {
  row_id id;
};
constinit lv::list<row, 4> rows;
constexpr auto ui = lv::column[lv::each(rows, &row::id)[lv::row[stamp{}]]];
void show(lv::obj screen) { auto root = lv::mount<ui>(screen); }
