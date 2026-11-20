#include <lv/list.hpp>
#include <lv/lv.hpp>
#include <lv/shown.hpp>
struct track {
  int id;
};
constinit lv::list<track, 4> tracks;
enum class look { small, large };
constinit lv::state<look> size{look::small};
constexpr auto a = lv::label();
constexpr auto ui = lv::column()[lv::each(tracks, &track::id)[lv::group()[lv::shown(size)[lv::is<look::small>(a), lv::otherwise()]]]];
void f(lv::obj parent) { auto root = lv::mount<ui>(parent); }
