#include <lv/lv.hpp>
#include <lv/shown.hpp>

// Two arms that name the same value, neither of them the first value named.
enum class page { home, settings, ink };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto b = lv::label();
constexpr auto c = lv::label();
constexpr auto ui = lv::group()[lv::shown(current)[lv::is<page::home>(a), lv::is<page::ink>(b), lv::is<page::settings, page::ink>(c)]];
