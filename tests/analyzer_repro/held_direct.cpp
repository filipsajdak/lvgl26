// Under -fanalyzer: the read through an lv::held, in a function that nothing calls with a known handle.
#include <lv/lv.hpp>
#include <lv/one_of.hpp>

constexpr auto ui = lv::column()[lv::label({.id = "title"})];
const char* read(lv::held<ui> p) { return lv_label_get_text(p->get<"title">().raw()); }

int main() { return 0; }
