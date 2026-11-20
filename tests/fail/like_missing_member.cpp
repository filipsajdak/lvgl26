#include <lv/lv.hpp>
#include <lv/repeat.hpp>
#include <array>
enum class subpage { none, print, scan };
enum class other { x };
struct level { int percent; };
struct icons { static constexpr char print[] = "P"; };
constexpr auto t = []<subpage P> { return lv::label({.text = lv::like<P, icons>}); };
constexpr auto ui = lv::row[lv::for_each<subpage, lv::but<subpage::none>>(t)];
void f(lv::obj s) { lv::mount<ui>(s); }
