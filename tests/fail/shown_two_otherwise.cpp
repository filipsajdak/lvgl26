#include <lv/lv.hpp>
#include <lv/shown.hpp>
enum class page { home, settings, ink };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto ui = lv::group()[lv::shown(current)[lv::otherwise(a), lv::otherwise()]];
