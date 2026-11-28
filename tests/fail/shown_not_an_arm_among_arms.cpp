#include <lv/lv.hpp>
#include <lv/shown.hpp>

// One arm of lv::is and a tree that is no arm.
enum class page { home, settings };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto b = lv::label();
constexpr auto ui = lv::group()[lv::shown(current)[lv::is<page::home>(a), b]];
