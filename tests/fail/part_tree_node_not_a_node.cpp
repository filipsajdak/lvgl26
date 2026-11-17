#include <lv/lv.hpp>
constexpr auto framed(lv::tree_node auto child) { return lv::group({})[child]; }
constexpr auto ui = framed("text");  // a node of a tree, such as lv::label({...})
