// A task tied to a widget needs allocation, in a project that does not allow it.
#include <lv/config.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{.allow = {.allocation = false}};

#include <lv/lv.hpp>

lv::task<> work() { co_return; }

void use(lv::obj owner) {
  lv::spawn(work(), owner);
}
