#include <lv/lv.hpp>
// The id comes first: a property before it is out of order, and GCC says so.
constexpr auto ui = lv::label({.text = "x", .id = "title"});  // order on purpose
