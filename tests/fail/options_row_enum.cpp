#include <lv/list.hpp>
#include <lv/lv.hpp>
enum class sheet { a4, letter };
struct job {
  sheet paper;
};
// A field of a row that holds an enum is no choice of options.
constexpr auto ui = lv::dropdown({.options = lv::options_of<sheet>, .selected = lv::bind(&job::paper)});
