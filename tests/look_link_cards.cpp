// The second file of the program: it names looks too, and mounts a tree of its own.
#include "look_link.hpp"

namespace {

constexpr auto card = lv::box({.style = lv::look<"card"> | lv::look<"tile-print">});

}  // namespace

lv::obj mount_card(lv::obj parent) {
  auto built = lv::mount<card>(parent);
  return built ? built->detach() : lv::obj{};
}
